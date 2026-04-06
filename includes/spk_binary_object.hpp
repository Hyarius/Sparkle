#pragma once

#include <span>
#include <vector>
#include <stdexcept>

#include "spk_binary_common.hpp"
#include "spk_synchronizable_trait.hpp"

namespace spk
{
	class BinaryField;

	class BinaryObject : public SynchronizableTrait
	{
	private:
		const BinaryLayoutMode _layoutMode;
		_binary_detail::Node _root;
		std::vector<std::uint8_t> _buffer;

		static _binary_detail::Node* _findChildByName(_binary_detail::Node*, std::string_view);
		_binary_detail::Node* _appendChild(_binary_detail::Node*, std::string_view, BinaryFieldKind);

		std::size_t _computeValueAlignment(std::size_t) const;
		std::size_t _computeArrayAlignment(std::size_t) const;
		std::size_t _computeArrayStride(std::size_t) const;

		void _layoutNode(_binary_detail::Node*, std::size_t);

	protected:
		void _synchronize() override;

	public:
		explicit BinaryObject(BinaryLayoutMode p_layoutMode = BinaryLayoutMode::Packed);

		BinaryField root();
		BinaryField operator[](std::string_view);

		BinaryField addValue(std::string_view, std::size_t);
		BinaryField addArray(std::string_view, std::size_t, std::size_t);
		BinaryField addObject(std::string_view);

		std::size_t size();
		std::span<std::uint8_t> buffer();

		friend class BinaryField;
	};
}