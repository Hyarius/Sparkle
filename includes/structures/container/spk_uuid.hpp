#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>

namespace spk
{
	class UUID
	{
	public:
		using Storage = std::array<std::uint8_t, 16>;

	private:
		Storage _bytes{};

	public:
		UUID();
		explicit UUID(const Storage &p_bytes);

		[[nodiscard]] static UUID generate();
		[[nodiscard]] static UUID null();

		[[nodiscard]] const Storage &bytes() const;
		[[nodiscard]] bool isNull() const;
		[[nodiscard]] std::string toString() const;

		[[nodiscard]] bool operator==(const UUID &p_other) const = default;
		[[nodiscard]] bool operator!=(const UUID &p_other) const = default;
	};

	std::ostream &operator<<(std::ostream &p_stream, const UUID &p_uuid);
}

namespace std
{
	template <>
	struct hash<spk::UUID>
	{
		[[nodiscard]] std::size_t operator()(const spk::UUID &p_uuid) const noexcept;
	};
}
