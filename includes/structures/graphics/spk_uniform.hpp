#pragma once

#include <array>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/graphics/opengl/spk_opengl_uniform.hpp"
#include "structures/graphics/opengl/spk_opengl_uniform_location.hpp"
#include "structures/graphics/spk_program.hpp"

namespace spk
{
	class UniformBase : public spk::SynchronizableTrait
	{
	protected:
		std::string _name;
		spk::Program &_program;
		mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::UniformLocation> _locationCache;

		UniformBase(std::string p_name, spk::Program &p_program);

		[[nodiscard]] spk::OpenGL::UniformLocation &_resolveAndValidate(
			GLenum p_expectedType,
			const char *p_expectedTypeName,
			std::size_t p_expectedCount) const;

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

		static StoredValueType toStoredValue(const ValueType &p_value)
		{
			return p_value;
		}
	};

	struct FloatUniformSpecification : UniformStorageSpecification<float>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT;
		static constexpr const char *TypeName = "float";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform1f(p_location, p_value);
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform1fv(p_location, p_count, p_values);
		}
	};

	struct BoolUniformSpecification
	{
		using ValueType = bool;
		using StoredValueType = GLint;
		static constexpr GLenum OpenGLType = GL_BOOL;
		static constexpr const char *TypeName = "bool";

		static StoredValueType toStoredValue(const ValueType &p_value)
		{
			return p_value ? 1 : 0;
		}
		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform1i(p_location, p_value ? 1 : 0);
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform1iv(p_location, p_count, p_values);
		}
	};

	struct IntUniformSpecification : UniformStorageSpecification<GLint>
	{
		static constexpr GLenum OpenGLType = GL_INT;
		static constexpr const char *TypeName = "int";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform1i(p_location, p_value);
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform1iv(p_location, p_count, p_values);
		}
	};

	struct UIntUniformSpecification : UniformStorageSpecification<GLuint>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT;
		static constexpr const char *TypeName = "uint";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform1ui(p_location, p_value);
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform1uiv(p_location, p_count, p_values);
		}
	};

	struct Vector2UniformSpecification : UniformStorageSpecification<std::array<float, 2>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_VEC2;
		static constexpr const char *TypeName = "vec2";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform2fv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform2fv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector3UniformSpecification : UniformStorageSpecification<std::array<float, 3>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_VEC3;
		static constexpr const char *TypeName = "vec3";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform3fv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform3fv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector4UniformSpecification : UniformStorageSpecification<std::array<float, 4>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_VEC4;
		static constexpr const char *TypeName = "vec4";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform4fv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform4fv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector2IntUniformSpecification : UniformStorageSpecification<std::array<GLint, 2>>
	{
		static constexpr GLenum OpenGLType = GL_INT_VEC2;
		static constexpr const char *TypeName = "ivec2";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform2iv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform2iv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector3IntUniformSpecification : UniformStorageSpecification<std::array<GLint, 3>>
	{
		static constexpr GLenum OpenGLType = GL_INT_VEC3;
		static constexpr const char *TypeName = "ivec3";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform3iv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform3iv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector4IntUniformSpecification : UniformStorageSpecification<std::array<GLint, 4>>
	{
		static constexpr GLenum OpenGLType = GL_INT_VEC4;
		static constexpr const char *TypeName = "ivec4";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform4iv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform4iv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector2UIntUniformSpecification : UniformStorageSpecification<std::array<GLuint, 2>>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT_VEC2;
		static constexpr const char *TypeName = "uvec2";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform2uiv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform2uiv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector3UIntUniformSpecification : UniformStorageSpecification<std::array<GLuint, 3>>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT_VEC3;
		static constexpr const char *TypeName = "uvec3";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform3uiv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform3uiv(p_location, p_count, p_values[0].data());
		}
	};

	struct Vector4UIntUniformSpecification : UniformStorageSpecification<std::array<GLuint, 4>>
	{
		static constexpr GLenum OpenGLType = GL_UNSIGNED_INT_VEC4;
		static constexpr const char *TypeName = "uvec4";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniform4uiv(p_location, 1, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniform4uiv(p_location, p_count, p_values[0].data());
		}
	};

	struct Matrix2x2UniformSpecification : UniformStorageSpecification<std::array<float, 4>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_MAT2;
		static constexpr const char *TypeName = "mat2";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniformMatrix2fv(p_location, 1, GL_FALSE, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniformMatrix2fv(p_location, p_count, GL_FALSE, p_values[0].data());
		}
	};

	struct Matrix3x3UniformSpecification : UniformStorageSpecification<std::array<float, 9>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_MAT3;
		static constexpr const char *TypeName = "mat3";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniformMatrix3fv(p_location, 1, GL_FALSE, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniformMatrix3fv(p_location, p_count, GL_FALSE, p_values[0].data());
		}
	};

	struct Matrix4x4UniformSpecification : UniformStorageSpecification<std::array<float, 16>>
	{
		static constexpr GLenum OpenGLType = GL_FLOAT_MAT4;
		static constexpr const char *TypeName = "mat4";

		static void activate(GLint p_location, const ValueType &p_value)
		{
			glUniformMatrix4fv(p_location, 1, GL_FALSE, p_value.data());
		}
		static void activateArray(GLint p_location, const StoredValueType *p_values, GLsizei p_count)
		{
			glUniformMatrix4fv(p_location, p_count, GL_FALSE, p_values[0].data());
		}
	};


	template <typename TSpec>
	class ScalarUniform : public UniformBase
	{
	public:
		using ValueType = typename TSpec::ValueType;

	private:
		ValueType _value{};

		void _synchronize() const override
		{
			spk::OpenGL::UniformLocation &loc =
				_resolveAndValidate(TSpec::OpenGLType, TSpec::TypeName, 1);
			TSpec::activate(loc.location, _value);
		}

	public:
		ScalarUniform(std::string p_name, spk::Program &p_program) :
			UniformBase(std::move(p_name), p_program)
		{
		}

		void set(const ValueType &p_value)
		{
			_value = p_value;
			requestSynchronization();
		}

		ScalarUniform &operator=(const ValueType &p_value)
		{
			set(p_value);
			return *this;
		}

		[[nodiscard]] const ValueType &value() const
		{
			return _value;
		}
	};


	template <typename TSpec>
	class ArrayUniform : public UniformBase
	{
	public:
		using ValueType = typename TSpec::ValueType;
		using StoredValueType = typename TSpec::StoredValueType;

	private:
		std::vector<StoredValueType> _values;

		void _synchronize() const override
		{
			spk::OpenGL::UniformLocation &loc =
				_resolveAndValidate(TSpec::OpenGLType, TSpec::TypeName, _values.size());
			TSpec::activateArray(loc.location, _values.data(), static_cast<GLsizei>(_values.size()));
		}

	public:
		ArrayUniform(std::string p_name, spk::Program &p_program, std::size_t p_count) :
			UniformBase(std::move(p_name), p_program),
			_values(p_count)
		{
			if (p_count == 0)
			{
				throw std::runtime_error("ArrayUniform: count cannot be zero");
			}
		}

		void set(const ValueType *p_values, std::size_t p_count)
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
				_values[i] = TSpec::toStoredValue(p_values[i]);
			}

			requestSynchronization();
		}

		void set(const std::vector<ValueType> &p_values)
		{
			set(p_values.data(), p_values.size());
		}
		void set(std::initializer_list<ValueType> p_values)
		{
			set(p_values.begin(), p_values.size());
		}
		ArrayUniform &operator=(const std::vector<ValueType> &p_values)
		{
			set(p_values);
			return *this;
		}
		ArrayUniform &operator=(std::initializer_list<ValueType> p_values)
		{
			set(p_values);
			return *this;
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
