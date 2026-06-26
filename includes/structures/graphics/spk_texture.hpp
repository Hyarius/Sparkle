#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/graphics/opengl/spk_cached_opengl_object_collection.hpp"
#include "structures/math/spk_vector2.hpp"

namespace spk
{
	namespace OpenGL
	{
		class Texture;
	}

	class Texture : public SynchronizableTrait
	{
	public:
		using ID = long;

		static constexpr ID InvalidID = -1;

		enum class Format
		{
			RGB,
			RGBA,
			BGR,
			BGRA,
			GreyLevel,
			DualChannel,
			Error
		};

		enum class Filtering
		{
			Nearest,
			Linear
		};

		enum class Wrap
		{
			Repeat,
			ClampToEdge,
			ClampToBorder
		};

		enum class Mipmap
		{
			Disable,
			Enable
		};

		struct Section
		{
			spk::Vector2 anchor;
			spk::Vector2 size;

			Section();
			Section(spk::Vector2 p_anchor, spk::Vector2 p_size);

			bool operator==(const Section &p_other) const noexcept;
			bool operator!=(const Section &p_other) const noexcept;

			static const Section whole;
		};

	private:
		struct Resource
		{
			std::uint64_t version = 0;
			std::vector<std::uint8_t> pixels;
			spk::Vector2UInt size{0, 0};
			Format format = Format::Error;
			Filtering filtering = Filtering::Nearest;
			Wrap wrap = Wrap::ClampToEdge;
			Mipmap mipmap = Mipmap::Enable;
		};

		struct SharedState
		{
			ID id = InvalidID;

			mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::Texture> gpu;

			mutable std::mutex dataMutex;
			std::shared_ptr<const Resource> resource;

			SharedState();
			SharedState(const SharedState &p_other);
			~SharedState();

			SharedState &operator=(const SharedState &p_other) = delete;
			SharedState(SharedState &&p_other) noexcept = delete;
			SharedState &operator=(SharedState &&p_other) noexcept = delete;
		};

		static std::deque<ID> &_availableIDs();
		static ID &_nextID();
		static ID _takeId();
		static void _releaseId(ID p_id);

		std::shared_ptr<SharedState> _state;

		void _ensureResource();
		[[nodiscard]] std::shared_ptr<const Resource> _resourceSnapshot() const;
		[[nodiscard]] const Resource &_resourceData() const;
		void _publishResource(const std::shared_ptr<Resource> &p_resource);
		static size_t _getBytesPerPixel(Format p_format);

	protected:
		void _synchronize() const override;

		friend class spk::OpenGL::Texture;

	public:
		Texture();
		virtual ~Texture() = default;

		Texture(const Texture &p_other);
		Texture &operator=(const Texture &p_other);

		Texture(Texture &&p_other) noexcept;
		Texture &operator=(Texture &&p_other) noexcept;

		[[nodiscard]] Texture clone() const;

		void setPixels(
			const std::vector<std::uint8_t> &p_data,
			const spk::Vector2UInt &p_size,
			Format p_format,
			Filtering p_filtering,
			Wrap p_wrap,
			Mipmap p_mipmap);

		void setPixels(
			const std::vector<std::uint8_t> &p_data,
			const spk::Vector2UInt &p_size,
			Format p_format);

		void setPixels(
			const std::uint8_t *p_data,
			const spk::Vector2UInt &p_size,
			Format p_format,
			Filtering p_filtering,
			Wrap p_wrap,
			Mipmap p_mipmap);

		void setPixels(
			const std::uint8_t *p_data,
			const spk::Vector2UInt &p_size,
			Format p_format);

		void setProperties(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap);

		[[nodiscard]] std::uint64_t version() const noexcept;

		[[nodiscard]] spk::OpenGL::Texture &gpu(const spk::RenderContext &p_context) const;
		[[nodiscard]] bool hasGpu(const spk::RenderContext &p_context) const noexcept;

		[[nodiscard]] ID id() const;
		[[nodiscard]] const std::vector<std::uint8_t> &pixels() const;
		[[nodiscard]] const spk::Vector2UInt &size() const;
		[[nodiscard]] Format format() const;
		[[nodiscard]] Filtering filtering() const;
		[[nodiscard]] Wrap wrap() const;
		[[nodiscard]] Mipmap mipmap() const;

		void saveAsPng(const std::filesystem::path &p_path) const;
	};
}
