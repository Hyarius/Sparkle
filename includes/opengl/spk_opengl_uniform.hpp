#pragma once

#include "opengl/spk_opengl_program.hpp"
#include "traits/spk_synchronizable_trait.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <GL/glew.h>

namespace spk
{
	class UniformBase : public spk::SynchronizableTrait
	{
	private:
		std::string _name;
		spk::Program& _program;
		mutable GLint _location = -1;
		mutable bool _locationResolved = false;
		mutable bool _validated = false;

	protected:
		UniformBase(std::string p_name, spk::Program& p_program) :
			_name(std::move(p_name)),
			_program(p_program)
		{
			requestSynchronization();
		}

		[[nodiscard]] GLint _locationValue() const
		{
			return _location;
		}

		[[nodiscard]] const std::string& _nameValue() const
		{
			return _name;
		}

		virtual bool _validateType(GLenum p_shaderType) const = 0;
		virtual const char* _expectedTypeName() const noexcept = 0;
		virtual std::size_t _expectedCount() const noexcept = 0;
		virtual void _onDataActivation() const = 0;

	private:
		void _synchronize() const final override
		{
			if (_locationResolved == false)
			{
				_location = _findUniformLocation(_program.id(), _name);
				_locationResolved = true;
				if (_location == -1)
				{
					throw std::runtime_error(
						"spk::UniformBase: uniform [" + _name + "] not found in program");
				}
			}
			_validateShaderDeclarationIfNeeded();
			_onDataActivation();
		}

		void _validateShaderDeclarationIfNeeded() const
		{
			if (_validated == true)
			{
				return;
			}

			_validateShaderDeclaration();
			_validated = true;
		}

		void _validateShaderDeclaration() const
		{
			const std::string normalizedName = _normalizeUniformName(_name);

			GLuint uniformIndex = GL_INVALID_INDEX;

			{
				const GLchar* uniformNames[] = {
					normalizedName.c_str()
				};

				glGetUniformIndices(
					_program.id(),
					1,
					uniformNames,
					&uniformIndex);
			}

			if (uniformIndex == GL_INVALID_INDEX)
			{
				const std::string arrayElementName = normalizedName + "[0]";

				const GLchar* uniformNames[] = {
					arrayElementName.c_str()
				};

				glGetUniformIndices(
					_program.id(),
					1,
					uniformNames,
					&uniformIndex);
			}

			if (uniformIndex == GL_INVALID_INDEX)
			{
				throw std::runtime_error(
					"spk::UniformBase: uniform [" + _name +
					"] not found in active uniform declarations");
			}

			GLint activeType = 0;

			glGetActiveUniformsiv(
				_program.id(),
				1,
				&uniformIndex,
				GL_UNIFORM_TYPE,
				&activeType);

			if (_validateType(static_cast<GLenum>(activeType)) == false)
			{
				throw std::runtime_error(
					"spk::UniformBase: uniform [" + _name +
					"] has a different shader type than expected. Expected [" +
					std::string(_expectedTypeName()) + "]");
			}

			GLint activeSize = 0;

			glGetActiveUniformsiv(
				_program.id(),
				1,
				&uniformIndex,
				GL_UNIFORM_SIZE,
				&activeSize);

			if (static_cast<std::size_t>(activeSize) < _expectedCount())
			{
				throw std::runtime_error(
					"UniformBase: uniform array [" + _name +
					"] is smaller than requested");
			}
		}

		static std::string _normalizeUniformName(const std::string& p_name)
		{
			const std::string arraySuffix = "[0]";

			if (p_name.size() >= arraySuffix.size() &&
				p_name.compare(p_name.size() - arraySuffix.size(), arraySuffix.size(), arraySuffix) == 0)
			{
				return p_name.substr(0, p_name.size() - arraySuffix.size());
			}

			return p_name;
		}

		static GLint _findUniformLocation(GLuint p_programId, const std::string& p_name)
		{
			GLint location = glGetUniformLocation(p_programId, p_name.c_str());

			if (location != -1)
			{
				return location;
			}

			const std::string arraySuffix = "[0]";

			if (p_name.size() >= arraySuffix.size() &&
				p_name.compare(p_name.size() - arraySuffix.size(), arraySuffix.size(), arraySuffix) == 0)
			{
				return -1;
			}

			const std::string arrayElementName = p_name + arraySuffix;
			return glGetUniformLocation(p_programId, arrayElementName.c_str());
		}

	public:
		virtual ~UniformBase() = default;

		[[nodiscard]] bool needsActivation() const noexcept
		{
			return needsSynchronization();
		}

		void activate()
		{
			synchronize();
		}

		void forceActivation()
		{
			forceSynchronization();
		}
	};

	template <typename TValue>
	struct UniformStorageSpecification
	{
		using ValueType = TValue;
		using StoredValueType = TValue;

		static StoredValueType toStoredValue(const ValueType& p_value)
		{
			return p_value;
		}
	};

	template <typename TSpecification>
	class ScalarUniform : public UniformBase
	{
	public:
		using ValueType = typename TSpecification::ValueType;

	private:
		ValueType _value{};

	protected:
		bool _validateType(GLenum p_shaderType) const override
		{
			return p_shaderType == TSpecification::OpenGLType;
		}

		const char* _expectedTypeName() const noexcept override
		{
			return TSpecification::TypeName;
		}

		std::size_t _expectedCount() const noexcept override
		{
			return 1;
		}

		void _onDataActivation() const override
		{
			TSpecification::activate(_locationValue(), _value);
		}

	public:
		ScalarUniform(std::string p_name, spk::Program& p_program) :
			UniformBase(std::move(p_name), p_program)
		{
		}

		void set(const ValueType& p_value)
		{
			_value = p_value;
			requestSynchronization();
		}

		ScalarUniform& operator=(const ValueType& p_value)
		{
			set(p_value);
			return *this;
		}

		[[nodiscard]] const ValueType& value() const
		{
			return _value;
		}
	};

	template <typename TSpecification>
	class ArrayUniform : public UniformBase
	{
	public:
		using ValueType = typename TSpecification::ValueType;
		using StoredValueType = typename TSpecification::StoredValueType;

	private:
		std::vector<StoredValueType> _values;

	protected:
		bool _validateType(GLenum p_shaderType) const override
		{
			return p_shaderType == TSpecification::OpenGLType;
		}

		const char* _expectedTypeName() const noexcept override
		{
			return TSpecification::TypeName;
		}

		std::size_t _expectedCount() const noexcept override
		{
			return _values.size();
		}

		void _onDataActivation() const override
		{
			TSpecification::activateArray(
				_locationValue(),
				_values.data(),
				static_cast<GLsizei>(_values.size()));
		}

	public:
		ArrayUniform(std::string p_name, spk::Program& p_program, std::size_t p_count) :
			UniformBase(std::move(p_name), p_program),
			_values(p_count)
		{
			if (p_count == 0)
			{
				throw std::runtime_error("ArrayUniform: count cannot be zero");
			}
		}

		void set(const ValueType* p_values, std::size_t p_count)
		{
			if (p_values == nullptr && p_count > 0)
			{
				throw std::runtime_error("ArrayUniform: null value array");
			}

			if (p_count != _values.size())
			{
				throw std::runtime_error("ArrayUniform: count mismatch");
			}

			for (std::size_t i = 0; i < p_count; i++)
			{
				_values[i] = TSpecification::toStoredValue(p_values[i]);
			}

			requestSynchronization();
		}

		void set(const std::vector<ValueType>& p_values)
		{
			set(p_values.data(), p_values.size());
		}

		void set(std::initializer_list<ValueType> p_values)
		{
			set(p_values.begin(), p_values.size());
		}

		ArrayUniform& operator=(const std::vector<ValueType>& p_values)
		{
			set(p_values);
			return *this;
		}

		ArrayUniform& operator=(std::initializer_list<ValueType> p_values)
		{
			set(p_values);
			return *this;
		}
	};

	struct FloatUniformSpecification : public UniformStorageSpecification<float>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT;
		static constexpr const char* TypeName = "float";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform1f(p_location, p_value);
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform1fv(p_location, p_count, p_values);
		}
	};

	struct BoolUniformSpecification
	{
		using ValueType = bool;
		using StoredValueType = GLint;

		static constexpr GLenum OpenGLType = GL_BOOL;
		static constexpr const char* TypeName = "bool";

		static StoredValueType toStoredValue(const ValueType& p_value)
		{
			return p_value ? 1 : 0;
		}

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform1i(p_location, p_value ? 1 : 0);
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform1iv(p_location, p_count, p_values);
		}
	};

	struct IntUniformSpecification : public UniformStorageSpecification<GLint>
	{
		static constexpr GLenum OpenGLType = GL_INT;
		static constexpr const char* TypeName = "int";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform1i(p_location, p_value);
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform1iv(p_location, p_count, p_values);
		}
	};

	struct UIntUniformSpecification : public UniformStorageSpecification<GLuint>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT;
		static constexpr const char* TypeName = "uint";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform1ui(p_location, p_value);
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform1uiv(p_location, p_count, p_values);
		}
	};

	struct Vector2UniformSpecification : public UniformStorageSpecification<std::array<float, 2>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_VEC2;
		static constexpr const char* TypeName = "vec2";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform2fv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform2fv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector3UniformSpecification : public UniformStorageSpecification<std::array<float, 3>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_VEC3;
		static constexpr const char* TypeName = "vec3";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform3fv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform3fv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector4UniformSpecification : public UniformStorageSpecification<std::array<float, 4>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_VEC4;
		static constexpr const char* TypeName = "vec4";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform4fv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform4fv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector2IntUniformSpecification : public UniformStorageSpecification<std::array<GLint, 2>>
	{
		static constexpr GLenum OpenGLType = GL_INT_VEC2;
		static constexpr const char* TypeName = "ivec2";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform2iv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform2iv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector3IntUniformSpecification : public UniformStorageSpecification<std::array<GLint, 3>>
	{
		static constexpr GLenum OpenGLType = GL_INT_VEC3;
		static constexpr const char* TypeName = "ivec3";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform3iv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform3iv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector4IntUniformSpecification : public UniformStorageSpecification<std::array<GLint, 4>>
	{
		static constexpr GLenum OpenGLType = GL_INT_VEC4;
		static constexpr const char* TypeName = "ivec4";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform4iv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform4iv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector2UIntUniformSpecification : public UniformStorageSpecification<std::array<GLuint, 2>>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT_VEC2;
		static constexpr const char* TypeName = "uvec2";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform2uiv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform2uiv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector3UIntUniformSpecification : public UniformStorageSpecification<std::array<GLuint, 3>>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT_VEC3;
		static constexpr const char* TypeName = "uvec3";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform3uiv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform3uiv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector4UIntUniformSpecification : public UniformStorageSpecification<std::array<GLuint, 4>>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT_VEC4;
		static constexpr const char* TypeName = "uvec4";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniform4uiv(p_location, 1, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniform4uiv(p_location, p_count, p_values[0].data());
		}
	};

	struct Matrix2x2UniformSpecification : public UniformStorageSpecification<std::array<float, 4>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_MAT2;
		static constexpr const char* TypeName = "mat2";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniformMatrix2fv(p_location, 1, GL_FALSE, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniformMatrix2fv(p_location, p_count, GL_FALSE, p_values[0].data());
		}
	};

	struct Matrix3x3UniformSpecification : public UniformStorageSpecification<std::array<float, 9>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_MAT3;
		static constexpr const char* TypeName = "mat3";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniformMatrix3fv(p_location, 1, GL_FALSE, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniformMatrix3fv(p_location, p_count, GL_FALSE, p_values[0].data());
		}
	};

	struct Matrix4x4UniformSpecification : public UniformStorageSpecification<std::array<float, 16>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_MAT4;
		static constexpr const char* TypeName = "mat4";

		static void activate(GLint p_location, const ValueType& p_value)
		{
			glUniformMatrix4fv(p_location, 1, GL_FALSE, p_value.data());
		}

		static void activateArray(GLint p_location, const StoredValueType* p_values, GLsizei p_count)
		{
			glUniformMatrix4fv(p_location, p_count, GL_FALSE, p_values[0].data());
		}
	};

	using FloatUniform = ScalarUniform<FloatUniformSpecification>;
	using BoolUniform = ScalarUniform<BoolUniformSpecification>;
	using IntUniform = ScalarUniform<IntUniformSpecification>;
	using UIntUniform = ScalarUniform<UIntUniformSpecification>;

	using Vector2Uniform = ScalarUniform<Vector2UniformSpecification>;
	using Vector3Uniform = ScalarUniform<Vector3UniformSpecification>;
	using Vector4Uniform = ScalarUniform<Vector4UniformSpecification>;

	using Vector2IntUniform = ScalarUniform<Vector2IntUniformSpecification>;
	using Vector3IntUniform = ScalarUniform<Vector3IntUniformSpecification>;
	using Vector4IntUniform = ScalarUniform<Vector4IntUniformSpecification>;

	using Vector2UIntUniform = ScalarUniform<Vector2UIntUniformSpecification>;
	using Vector3UIntUniform = ScalarUniform<Vector3UIntUniformSpecification>;
	using Vector4UIntUniform = ScalarUniform<Vector4UIntUniformSpecification>;

	using Matrix2x2Uniform = ScalarUniform<Matrix2x2UniformSpecification>;
	using Matrix3x3Uniform = ScalarUniform<Matrix3x3UniformSpecification>;
	using Matrix4x4Uniform = ScalarUniform<Matrix4x4UniformSpecification>;

	using FloatArrayUniform = ArrayUniform<FloatUniformSpecification>;
	using BoolArrayUniform = ArrayUniform<BoolUniformSpecification>;
	using IntArrayUniform = ArrayUniform<IntUniformSpecification>;
	using UIntArrayUniform = ArrayUniform<UIntUniformSpecification>;

	using Vector2ArrayUniform = ArrayUniform<Vector2UniformSpecification>;
	using Vector3ArrayUniform = ArrayUniform<Vector3UniformSpecification>;
	using Vector4ArrayUniform = ArrayUniform<Vector4UniformSpecification>;

	using Vector2IntArrayUniform = ArrayUniform<Vector2IntUniformSpecification>;
	using Vector3IntArrayUniform = ArrayUniform<Vector3IntUniformSpecification>;
	using Vector4IntArrayUniform = ArrayUniform<Vector4IntUniformSpecification>;

	using Vector2UIntArrayUniform = ArrayUniform<Vector2UIntUniformSpecification>;
	using Vector3UIntArrayUniform = ArrayUniform<Vector3UIntUniformSpecification>;
	using Vector4UIntArrayUniform = ArrayUniform<Vector4UIntUniformSpecification>;

	using Matrix2x2ArrayUniform = ArrayUniform<Matrix2x2UniformSpecification>;
	using Matrix3x3ArrayUniform = ArrayUniform<Matrix3x3UniformSpecification>;
	using Matrix4x4ArrayUniform = ArrayUniform<Matrix4x4UniformSpecification>;
}
