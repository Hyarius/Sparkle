# Review 001 — `includes/structures/container/spk_binary_field.hpp`

- File: [includes/structures/container/spk_binary_field.hpp](<../includes/structures/container/spk_binary_field.hpp>)
- Done [X]
- Remarks and requested changes:
Refactor spk::BinaryField by separating the binary schema from the accessed data.

Create two classes:

BinaryLayout, responsible only for describing fields, offsets, sizes, nested objects, and arrays.
BinaryView, responsible for applying a BinaryLayout to a byte buffer and reading or writing its values.

The same BinaryLayout must be reusable with multiple buffers.

Simplify the internal representation:

Replace section IDs with stable node pointers.
Store offsets relative to the parent only.
Remove absoluteOffset and parent.
Do not create one node per array element. Store one shared element description and calculate the selected element offset when operator[] is called.
Remove invalid/default states when possible.
Prefer explicit read<T>() and write<T>() methods instead of the templated assignment operator.
Use std::span<std::byte> for writable data and support a const/read-only equivalent.

Expected usage:

spk::BinaryLayout layout(sizeof(Packet));
layout.addValue<std::uint32_t>("id", offsetof(Packet, id));
layout.addArray<float>("weights", offsetof(Packet, weights), 3);

spk::BinaryView view(buffer, layout);

view["id"].write<std::uint32_t>(42);
view["weights"][1].write<float>(0.5f);

const float value = view["weights"][1].read<float>();

Preserve bounds validation, duplicate-name validation, trivially-copyable type checks, and clear exceptions.

