#include "structures/container/spk_uuid.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <random>
#include <sstream>

namespace spk
{
	UUID::UUID() = default;

	UUID::UUID(const Storage &p_bytes) :
		_bytes(p_bytes)
	{
	}

	UUID UUID::generate()
	{
		static thread_local std::mt19937_64 generator(std::random_device{}());
		std::uniform_int_distribution<std::uint32_t> distribution(0u, 255u);

		Storage bytes{};
		for (std::uint8_t &byte : bytes)
		{
			byte = static_cast<std::uint8_t>(distribution(generator));
		}

		bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0Fu) | 0x40u);
		bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3Fu) | 0x80u);

		return UUID(bytes);
	}

	UUID UUID::null()
	{
		return UUID();
	}

	const UUID::Storage &UUID::bytes() const
	{
		return _bytes;
	}

	bool UUID::isNull() const
	{
		return std::all_of(_bytes.begin(), _bytes.end(), [](std::uint8_t p_byte) {
			return p_byte == 0u;
		});
	}

	std::string UUID::toString() const
	{
		std::ostringstream stream;
		stream << std::hex << std::setfill('0');

		for (std::size_t i = 0; i < _bytes.size(); ++i)
		{
			if (i == 4u || i == 6u || i == 8u || i == 10u)
			{
				stream << '-';
			}

			stream << std::setw(2) << static_cast<unsigned int>(_bytes[i]);
		}

		return stream.str();
	}

	std::ostream &operator<<(std::ostream &p_stream, const UUID &p_uuid)
	{
		p_stream << p_uuid.toString();
		return p_stream;
	}
}

namespace std
{
	std::size_t hash<spk::UUID>::operator()(const spk::UUID &p_uuid) const noexcept
	{
		std::size_t result = 1469598103934665603ull;

		for (std::uint8_t byte : p_uuid.bytes())
		{
			result ^= static_cast<std::size_t>(byte);
			result *= 1099511628211ull;
		}

		return result;
	}
}
