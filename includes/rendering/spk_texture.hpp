#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <vector>

#include "math/spk_vector2.hpp"
#include "traits/spk_synchronizable_trait.hpp"

namespace spk
{
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

			bool operator==(const Section& p_other) const noexcept;
			bool operator!=(const Section& p_other) const noexcept;

			static const Section whole;
		};

	private:
		static std::deque<ID>& _availableIDs();
		static ID& _nextID();

		static ID _takeId();
		static void _releaseId(ID p_id);

		ID _id;

		std::vector<uint8_t> _pixels;
		spk::Vector2UInt _size;
		Format _format;
		Filtering _filtering;
		Wrap _wrap;
		Mipmap _mipmap;

		size_t _getBytesPerPixel(Format p_format) const;

	protected:
		void _synchronize() const override;

	public:
		Texture();
		virtual ~Texture();

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		Texture(Texture&&) noexcept;
		Texture& operator=(Texture&&) noexcept;

		void setPixels(
			const std::vector<uint8_t>& p_data,
			const spk::Vector2UInt& p_size,
			Format p_format,
			Filtering p_filtering,
			Wrap p_wrap,
			Mipmap p_mipmap);

		void setPixels(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size, Format p_format);

		void setPixels(
			const uint8_t* p_data,
			const spk::Vector2UInt& p_size,
			Format p_format,
			Filtering p_filtering,
			Wrap p_wrap,
			Mipmap p_mipmap);

		void setPixels(const uint8_t* p_data, const spk::Vector2UInt& p_size, Format p_format);

		void setProperties(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap);

		ID id() const;
		const std::vector<uint8_t>& pixels() const;
		const spk::Vector2UInt& size() const;
		Format format() const;
		Filtering filtering() const;
		Wrap wrap() const;
		Mipmap mipmap() const;

		void saveAsPng(const std::filesystem::path& p_path) const;
	};
}
