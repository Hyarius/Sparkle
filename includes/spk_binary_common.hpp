#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace spk
{
	enum class BinaryFieldKind
	{
		Value,
		Object,
		Array
	};

	enum class BinaryLayoutMode
	{
		Packed,
		ScalarAligned,
		Std140,
		Std430
	};

	namespace _binary_detail
	{
		struct Node
		{
			std::string name;
			BinaryFieldKind kind = BinaryFieldKind::Value;

			std::size_t offset = 0;
			std::size_t size = 0;
			std::size_t alignment = 1;
			std::size_t stride = 0;
			std::size_t count = 0;

			std::size_t valueSize = 0;
			std::size_t elementSize = 0;

			Node* parent = nullptr;
			std::vector<std::unique_ptr<Node>> children;
		};

		inline std::size_t alignUp(const std::size_t p_value, const std::size_t p_alignment)
		{
			if (p_alignment <= 1)
				return p_value;

			const std::size_t remainder = p_value % p_alignment;
			return (remainder == 0) ? p_value : (p_value + (p_alignment - remainder));
		}

		inline std::size_t computeScalarAlignedAlignment(const std::size_t p_size)
		{
			if (p_size <= 1) return 1;
			if (p_size <= 2) return 2;
			if (p_size <= 4) return 4;
			if (p_size <= 8) return 8;
			return 16;
		}

		inline std::size_t computeStdAlignmentFromSize(const std::size_t p_size)
		{
			if (p_size <= 4) return 4;
			if (p_size <= 8) return 8;
			return 16;
		}
	}
}