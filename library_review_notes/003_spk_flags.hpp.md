# Review 003 — `includes/structures/container/spk_flags.hpp`

- File: [includes/structures/container/spk_flags.hpp](<../includes/structures/container/spk_flags.hpp>)
- Done [X]
- Remarks and requested changes:
Refactor spk::Flags to make the distinction between replacing the current mask and modifying individual flags explicit.

Use the following mutation API:

flags.set(MyFlag::A);                 // Replace the complete mask with A.
flags.set({MyFlag::A, MyFlag::B});    // Replace the complete mask with A | B.
flags.set(otherFlags);                // Replace the complete mask.

flags.add(MyFlag::C);                 // Add C without modifying other flags.
flags.add(otherFlags);

flags.remove(MyFlag::A);              // Remove A without modifying other flags.
flags.remove(otherFlags);

flags.toggle(MyFlag::B);
flags.toggle(otherFlags);

flags.clear();                        // Reset the complete mask to zero.

Remove the current unset() and reset() methods because they duplicate the behavior of remove().

Add set() overloads for:

A single enum value.
Another Flags instance.
An std::initializer_list<TFlagType>.
Optionally, a raw mask through an explicitly named setRaw() method.

The expected semantics are:

Flags<MyFlag> flags = {MyFlag::A, MyFlag::B};

flags.set(MyFlag::C);
// Contains only C.

flags.add(MyFlag::A);
// Contains C and A.

flags.remove(MyFlag::C);
// Contains only A.

Improve the query API by replacing ambiguous names with explicit all/any semantics:

flags.contains(MyFlag::A);
flags.containsAll(otherFlags);
flags.containsAny(otherFlags);
flags.any();
flags.none();

A single enum overload of contains() should test whether that complete enum mask is present. This remains compatible with enums whose values may combine multiple bits.

Consider retaining has() and testAny() temporarily as deprecated wrappers if API compatibility is important.

Review the following additional points:

Remove unnecessary includes such as <iostream> from the core header if stream operators can be moved into a separate header.
Keep all operations constexpr and noexcept.
Keep construction from a single enum and an initializer list.
Keep raw-mask construction explicit.
Add a Flags64 alias using std::uint64_t.
Consider adding empty() as a readable alias for none().
Ensure zero-valued flags are handled deliberately: contains(Zero) should not accidentally be interpreted as every mask containing the flag unless that behavior is explicitly intended.
Consider whether operator~ should invert every storage bit or only the bits declared valid for the enum. If there is no known valid-bit mask, document that it inverts the complete storage value.
Keep bitwise operators consistent with the mutation methods:
| corresponds to add().
& computes an intersection.
^ corresponds to toggle().

The final API should prioritize clear semantics and avoid multiple methods that perform the same operation.

