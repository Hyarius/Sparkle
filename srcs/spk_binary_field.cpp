#include "spk_binary_field.hpp"

#include <limits>

namespace spk
{
	BinaryField::BinaryField(const std::shared_ptr<std::vector<Section>>& p_sections, std::size_t p_sectionID) :
		_sections(p_sections),
		_sectionID(p_sectionID)
	{
	}

	BinaryField::Section& BinaryField::_section()
	{
		if (isValid() == false)
		{
			throw std::runtime_error("Invalid BinaryField.");
		}

		return (*_sections)[_sectionID];
	}

	const BinaryField::Section& BinaryField::_section() const
	{
		if (isValid() == false)
		{
			throw std::runtime_error("Invalid BinaryField.");
		}

		return (*_sections)[_sectionID];
	}

	std::size_t BinaryField::_findChild(std::string_view p_name) const
	{
		const Section& parent = _section();
		for (const std::size_t childID : parent.children)
		{
			const Section& child = (*_sections)[childID];
			if (child.name == p_name)
			{
				return childID;
			}
		}

		return invalidSectionID;
	}

	BinaryField BinaryField::_addSection(
		std::string_view p_name,
		std::size_t p_offset,
		std::size_t p_size,
		BinaryField::Kind p_kind)
	{
		const Section& parent = _section();
		const Kind parentKind = parent.kind;
		std::uint8_t* const parentData = parent.data;
		const std::size_t parentSize = parent.size;

		if (parentKind == Kind::Value)
		{
			throw std::runtime_error("BinaryField children can only be added to object or array fields.");
		}

		if (parentKind == Kind::Array)
		{
			throw std::runtime_error("BinaryField named children cannot be added to array fields.");
		}

		if (p_name.empty())
		{
			throw std::runtime_error("BinaryField child name cannot be empty.");
		}

		if (_findChild(p_name) != invalidSectionID)
		{
			throw std::runtime_error("BinaryField child name already exists.");
		}

		if (p_size == 0)
		{
			throw std::runtime_error("BinaryField child size cannot be zero.");
		}

		if (p_offset > parentSize || p_size > parentSize - p_offset)
		{
			throw std::runtime_error("BinaryField child exceeds parent bounds.");
		}

		const std::size_t sectionID = _sections->size();
		_sections->emplace_back();
		Section& section = (*_sections)[sectionID];
		section.name = std::string(p_name);
		section.kind = p_kind;
		section.data = parentData + p_offset;
		section.size = p_size;
		section.offset = p_offset;
		section.parent = _sectionID;

		(*_sections)[_sectionID].children.push_back(sectionID);
		return BinaryField(_sections, sectionID);
	}

	BinaryField::BinaryField(std::uint8_t* p_data, std::size_t p_size)
	{
		if (p_data == nullptr && p_size != 0)
		{
			throw std::runtime_error("BinaryField cannot be constructed with null data and non-zero size.");
		}

		_sections = std::make_shared<std::vector<Section>>();
		Section& root = _sections->emplace_back();
		root.name = "<root>";
		root.kind = Kind::Object;
		root.data = p_data;
		root.size = p_size;
		root.offset = 0;
		_sectionID = 0;
	}

	bool BinaryField::isValid() const
	{
		return _sections != nullptr && _sectionID < _sections->size();
	}

	std::string_view BinaryField::name() const
	{
		return _section().name;
	}

	std::size_t BinaryField::size() const
	{
		return _section().size;
	}

	std::size_t BinaryField::offset() const
	{
		return _section().offset;
	}

	std::size_t BinaryField::count() const
	{
		return _section().count;
	}

	std::size_t BinaryField::elementSize() const
	{
		return _section().elementSize;
	}

	std::uint8_t* BinaryField::data()
	{
		return _section().data;
	}

	const std::uint8_t* BinaryField::data() const
	{
		return _section().data;
	}

	BinaryField BinaryField::addValue(std::string_view p_name, std::size_t p_offset, std::size_t p_size)
	{
		return _addSection(p_name, p_offset, p_size, Kind::Value);
	}

	BinaryField BinaryField::addObject(std::string_view p_name, std::size_t p_offset, std::size_t p_size)
	{
		return _addSection(p_name, p_offset, p_size, Kind::Object);
	}

	BinaryField BinaryField::addArray(std::string_view p_name, std::size_t p_offset, std::size_t p_count, std::size_t p_elementSize)
	{
		if (p_elementSize == 0)
		{
			throw std::runtime_error("BinaryField array element size cannot be zero.");
		}

		if (p_count > std::numeric_limits<std::size_t>::max() / p_elementSize)
		{
			throw std::runtime_error("BinaryField array size overflow.");
		}

		const std::size_t arraySize = p_count * p_elementSize;
		BinaryField array = _addSection(p_name, p_offset, arraySize, Kind::Array);
		array._section().count = p_count;
		array._section().elementSize = p_elementSize;

		for (std::size_t index = 0; index < p_count; ++index)
		{
			const std::size_t sectionID = _sections->size();
			_sections->emplace_back();
			Section& element = (*_sections)[sectionID];
			element.name = std::to_string(index);
			element.kind = Kind::Value;
			element.data = array._section().data + index * p_elementSize;
			element.size = p_elementSize;
			element.offset = index * p_elementSize;
			element.parent = array._sectionID;
			array._section().children.push_back(sectionID);
		}

		return array;
	}

	BinaryField BinaryField::operator[](std::string_view p_name)
	{
		const Section& section = _section();
		if (section.kind != Kind::Object)
		{
			throw std::runtime_error("BinaryField named lookup can only be used on object fields.");
		}

		const std::size_t childID = _findChild(p_name);
		if (childID == invalidSectionID)
		{
			throw std::runtime_error("BinaryField named lookup could not find the requested child.");
		}

		return BinaryField(_sections, childID);
	}

	BinaryField BinaryField::operator[](std::size_t p_index)
	{
		const Section& section = _section();
		if (section.kind != Kind::Array)
		{
			throw std::runtime_error("BinaryField indexed lookup can only be used on array fields.");
		}

		if (p_index >= section.children.size())
		{
			throw std::runtime_error("BinaryField indexed lookup received an out of range index.");
		}

		return BinaryField(_sections, section.children[p_index]);
	}

	std::span<std::uint8_t> BinaryField::bytes()
	{
		Section& section = _section();
		return std::span<std::uint8_t>(section.data, section.size);
	}

	std::span<const std::uint8_t> BinaryField::bytes() const
	{
		const Section& section = _section();
		return std::span<const std::uint8_t>(section.data, section.size);
	}
}
