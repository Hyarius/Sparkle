#include "structures/graphics/spk_texture.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <memory>
#include <stdexcept>
#include <utility>

#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	namespace
	{
		void validatePixelDataFormat(spk::Texture::Format p_format)
		{
			if (spk::Texture::isDepthFormat(p_format) || spk::Texture::isDepthStencilFormat(p_format))
			{
				throw std::invalid_argument("Depth formats are GPU render-target formats and cannot contain CPU pixel data");
			}
		}
	}

	std::deque<Texture::ID> &Texture::_availableIDs()
	{
		static std::deque<ID> ids;
		return ids;
	}

	Texture::ID &Texture::_nextID()
	{
		static ID next = 0;
		return next;
	}

	Texture::ID Texture::_takeId()
	{
		auto &available = _availableIDs();
		if (available.empty() == false)
		{
			ID result = available.back();
			available.pop_back();
			return result;
		}

		return _nextID()++;
	}

	void Texture::_releaseId(ID p_id)
	{
		if (p_id == InvalidID)
		{
			return;
		}

		_availableIDs().push_back(p_id);
	}

	Texture::SharedState::SharedState() :
		id(Texture::_takeId()),
		resource(std::make_shared<Resource>())
	{
	}

	Texture::SharedState::SharedState(const SharedState &p_other) :
		id(Texture::_takeId())
	{
		std::shared_ptr<Resource> copiedResource;

		{
			std::scoped_lock lock(p_other.dataMutex);
			copiedResource = std::make_shared<Resource>(*p_other.resource);
		}

		if (copiedResource->version > 0)
		{
			copiedResource->version = 1;
		}

		resource = std::move(copiedResource);
	}

	Texture::SharedState::~SharedState()
	{
		Texture::_releaseId(id);
	}

	size_t Texture::_getBytesPerPixel(Format p_format)
	{
		switch (p_format)
		{
		case Format::GreyLevel:
			return 1;
		case Format::DualChannel:
			return 2;
		case Format::RGB:
		case Format::BGR:
			return 3;
		case Format::RGBA:
		case Format::BGRA:
			return 4;
		default:
			return 0;
		}
	}

	void Texture::_ensureResource()
	{
		if (_state == nullptr)
		{
			_state = std::make_shared<SharedState>();
		}
	}

	std::shared_ptr<const Texture::Resource> Texture::_resourceSnapshot() const
	{
		if (_state == nullptr)
		{
			return std::make_shared<Resource>();
		}

		std::scoped_lock lock(_state->dataMutex);
		return _state->resource != nullptr ? _state->resource : std::make_shared<Resource>();
	}

	const Texture::Resource &Texture::_resourceData() const
	{
		static const Resource emptyResource;

		if (_state == nullptr)
		{
			return emptyResource;
		}

		std::scoped_lock lock(_state->dataMutex);
		return _state->resource != nullptr ? *_state->resource : emptyResource;
	}

	void Texture::_publishResource(const std::shared_ptr<Resource> &p_resource)
	{
		if (p_resource == nullptr)
		{
			return;
		}

		_ensureResource();

		{
			std::scoped_lock lock(_state->dataMutex);
			const std::uint64_t currentVersion = _state->resource != nullptr ? _state->resource->version : 0;
			p_resource->version = currentVersion + 1;
			_state->resource = p_resource;
		}

		requestSynchronization();
	}

	void Texture::_synchronize() const
	{
		spk::RenderContext *ctx = spk::RenderContext::current();
		if (ctx != nullptr && ctx->supportsOpenGLCommands() == true)
		{
			(void)gpu(*ctx);
		}
	}

	spk::OpenGL::Texture &Texture::gpu(const spk::RenderContext &p_context) const
	{
		if (_state == nullptr)
		{
			throw std::runtime_error("Cannot upload a moved-from texture");
		}

		return _state->gpu.resolve(
			p_context,
			version(),
			[this]() {
				return std::make_unique<spk::OpenGL::Texture>(*this);
			});
	}

	bool Texture::hasGpu(const spk::RenderContext &p_context) const noexcept
	{
		if (_state == nullptr)
		{
			return false;
		}

		const spk::OpenGL::Texture *object = _state->gpu.find(p_context);
		return object != nullptr && object->version() == version();
	}

	const Texture::Section Texture::Section::whole = Texture::Section({0.0f, 0.0f}, {1.0f, 1.0f});

	Texture::Section::Section() :
		anchor(0.0f, 0.0f),
		size(0.0f, 0.0f)
	{
	}

	Texture::Section::Section(spk::Vector2 p_anchor, spk::Vector2 p_size) :
		anchor(p_anchor),
		size(p_size)
	{
	}

	bool Texture::Section::operator==(const Section &p_other) const noexcept
	{
		return anchor == p_other.anchor && size == p_other.size;
	}

	bool Texture::Section::operator!=(const Section &p_other) const noexcept
	{
		return (*this == p_other) == false;
	}

	Texture::Texture() :
		_state(std::make_shared<SharedState>())
	{
	}

	Texture::Texture(const Texture &p_other) :
		_state(p_other._state)
	{
	}

	Texture &Texture::operator=(const Texture &p_other)
	{
		if (this != &p_other)
		{
			_state = p_other._state;
		}

		return *this;
	}

	Texture::Texture(Texture &&p_other) noexcept :
		SynchronizableTrait(std::move(p_other)),
		_state(std::move(p_other._state))
	{
	}

	Texture &Texture::operator=(Texture &&p_other) noexcept
	{
		if (this != &p_other)
		{
			SynchronizableTrait::operator=(std::move(p_other));
			_state = std::move(p_other._state);
		}

		return *this;
	}

	Texture Texture::clone() const
	{
		Texture result;
		if (_state == nullptr)
		{
			return result;
		}

		result._state = std::make_shared<SharedState>(*_state);

		if (result.version() > 0)
		{
			result.requestSynchronization();
		}

		return result;
	}

	bool Texture::isColorFormat(Format p_format) noexcept
	{
		switch (p_format)
		{
		case Format::RGB:
		case Format::RGBA:
		case Format::BGR:
		case Format::BGRA:
		case Format::GreyLevel:
		case Format::DualChannel:
			return true;
		default:
			return false;
		}
	}

	bool Texture::isDepthFormat(Format p_format) noexcept
	{
		return p_format == Format::Depth24 || p_format == Format::Depth32F;
	}

	bool Texture::isDepthStencilFormat(Format p_format) noexcept
	{
		return p_format == Format::Depth24Stencil8;
	}

	void Texture::setPixels(
		const std::vector<std::uint8_t> &p_data,
		const spk::Vector2UInt &p_size,
		Format p_format,
		Filtering p_filtering,
		Wrap p_wrap,
		Mipmap p_mipmap)
	{
		validatePixelDataFormat(p_format);
		auto resource = std::make_shared<Resource>();
		resource->pixels = p_data;
		resource->size = p_size;
		resource->format = p_format;
		resource->filtering = p_filtering;
		resource->wrap = p_wrap;
		resource->mipmap = p_mipmap;

		_publishResource(resource);
	}

	void Texture::setPixels(
		const std::vector<std::uint8_t> &p_data,
		const spk::Vector2UInt &p_size,
		Format p_format)
	{
		validatePixelDataFormat(p_format);
		std::shared_ptr<const Resource> currentResource = _resourceSnapshot();

		auto resource = std::make_shared<Resource>();
		resource->pixels = p_data;
		resource->size = p_size;
		resource->format = p_format;
		resource->filtering = currentResource->filtering;
		resource->wrap = currentResource->wrap;
		resource->mipmap = currentResource->mipmap;

		_publishResource(resource);
	}

	void Texture::setPixels(
		const std::uint8_t *p_data,
		const spk::Vector2UInt &p_size,
		Format p_format,
		Filtering p_filtering,
		Wrap p_wrap,
		Mipmap p_mipmap)
	{
		validatePixelDataFormat(p_format);
		auto resource = std::make_shared<Resource>();
		resource->size = p_size;
		resource->format = p_format;
		resource->filtering = p_filtering;
		resource->wrap = p_wrap;
		resource->mipmap = p_mipmap;

		const size_t byteCount = p_size.x * p_size.y * _getBytesPerPixel(p_format);
		if (p_data != nullptr)
		{
			resource->pixels.assign(p_data, p_data + byteCount);
		}
		else
		{
			resource->pixels.assign(byteCount, 0);
		}

		_publishResource(resource);
	}

	void Texture::setPixels(
		const std::uint8_t *p_data,
		const spk::Vector2UInt &p_size,
		Format p_format)
	{
		validatePixelDataFormat(p_format);
		std::shared_ptr<const Resource> currentResource = _resourceSnapshot();

		auto resource = std::make_shared<Resource>();
		resource->size = p_size;
		resource->format = p_format;
		resource->filtering = currentResource->filtering;
		resource->wrap = currentResource->wrap;
		resource->mipmap = currentResource->mipmap;

		const size_t byteCount = p_size.x * p_size.y * _getBytesPerPixel(p_format);
		if (p_data != nullptr)
		{
			resource->pixels.assign(p_data, p_data + byteCount);
		}
		else
		{
			resource->pixels.assign(byteCount, 0);
		}

		_publishResource(resource);
	}

	void Texture::allocateRenderTarget(
		const spk::Vector2UInt &p_size,
		Format p_format,
		Filtering p_filtering,
		Wrap p_wrap)
	{
		auto resource = std::make_shared<Resource>();
		resource->size = p_size;
		resource->format = p_format;
		resource->filtering = p_filtering;
		resource->wrap = p_wrap;
		resource->mipmap = Mipmap::Disable;
		resource->contentSource = ContentSource::RenderTarget;

		_publishResource(resource);
	}

	void Texture::setProperties(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap)
	{
		std::shared_ptr<const Resource> currentResource = _resourceSnapshot();
		auto resource = std::make_shared<Resource>();
		resource->pixels = currentResource->pixels;
		resource->size = currentResource->size;
		resource->format = currentResource->format;
		resource->contentSource = currentResource->contentSource;
		resource->filtering = p_filtering;
		resource->wrap = p_wrap;
		resource->mipmap = p_mipmap;

		_publishResource(resource);
	}

	std::uint64_t Texture::version() const noexcept
	{
		return _resourceSnapshot()->version;
	}

	Texture::ID Texture::id() const
	{
		if (_state == nullptr)
		{
			return InvalidID;
		}

		return _state->id;
	}

	const std::vector<std::uint8_t> &Texture::pixels() const
	{
		if (_resourceData().contentSource == ContentSource::RenderTarget)
		{
			throw std::runtime_error("Render-target textures do not expose CPU pixel data");
		}
		return _resourceData().pixels;
	}

	const spk::Vector2UInt &Texture::size() const
	{
		return _resourceData().size;
	}

	Texture::Format Texture::format() const
	{
		return _resourceData().format;
	}

	Texture::ContentSource Texture::contentSource() const
	{
		return _resourceData().contentSource;
	}

	bool Texture::isRenderTarget() const
	{
		return contentSource() == ContentSource::RenderTarget;
	}

	Texture::Filtering Texture::filtering() const
	{
		return _resourceData().filtering;
	}

	Texture::Wrap Texture::wrap() const
	{
		return _resourceData().wrap;
	}

	Texture::Mipmap Texture::mipmap() const
	{
		return _resourceData().mipmap;
	}

	void Texture::saveAsPng(const std::filesystem::path &p_path) const
	{
		std::shared_ptr<const Resource> resource = _resourceSnapshot();
		if (resource->contentSource == ContentSource::RenderTarget ||
			isDepthFormat(resource->format) || isDepthStencilFormat(resource->format))
		{
			throw std::runtime_error("Render-target and depth textures cannot be exported as PNG files.");
		}

		if (resource->pixels.empty() || resource->size.x == 0 || resource->size.y == 0)
		{
			throw std::runtime_error("Cannot save texture: no pixel data.");
		}

		const int channels = static_cast<int>(_getBytesPerPixel(resource->format));
		if (channels == 0)
		{
			throw std::runtime_error("Unsupported texture format for PNG export.");
		}

		if (p_path.has_parent_path())
		{
			std::filesystem::create_directories(p_path.parent_path());
		}

		const int stride = static_cast<int>(resource->size.x) * channels;
		if (stbi_write_png(
				p_path.string().c_str(),
				static_cast<int>(resource->size.x),
				static_cast<int>(resource->size.y),
				channels,
				resource->pixels.data(),
				stride) == 0)
		{
			throw std::runtime_error("Failed to write PNG file at: " + p_path.string());
		}
	}
}
