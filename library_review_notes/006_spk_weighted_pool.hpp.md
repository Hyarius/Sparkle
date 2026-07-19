# Review 006 — `includes/structures/container/spk_weighted_pool.hpp`

- File: [includes/structures/container/spk_weighted_pool.hpp](<../includes/structures/container/spk_weighted_pool.hpp>)
- Done [X]
- Remarks and requested changes:


Refactor `spk::WeightedPool<TValue>` to support efficient weighted selection using cumulative weights and `std::upper_bound`.

The pool is intended to contain hundreds or thousands of entries and may be queried frequently. Selection must therefore use an `O(log n)` binary search instead of the current `O(n)` linear scan.

### Internal representation

Keep the existing `Entry` structure:

```cpp
struct Entry
{
	TValue value;
	double weight = 1.0;
};
```

Store entries in their insertion order:

```cpp
std::vector<Entry> _entries;
```

Add a second vector containing the cumulative upper boundary of every entry:

```cpp
std::vector<double> _cumulativeWeights;
double _totalWeight = 0.0;
```

For entries with weights:

```text
2.0, 5.0, 3.0
```

the cumulative vector must contain:

```text
2.0, 7.0, 10.0
```

The cumulative vector must always have the same size as `_entries`, and its last value must equal `_totalWeight`.

### Adding entries

Keep:

```cpp
void add(TValue p_value, double p_weight = 1.0);
```

Validate that the weight is:

* Finite.
* Strictly positive.
* Representable in the accumulated total.

Calculate the new total before modifying the vectors:

```cpp
const double newTotalWeight = _totalWeight + p_weight;
```

Reject the operation if:

```cpp
!std::isfinite(newTotalWeight)
```

or if floating-point precision causes:

```cpp
newTotalWeight == _totalWeight
```

The second condition means the new entry would have a positive weight but would never be selectable.

Preserve strong exception safety. `_entries`, `_cumulativeWeights`, and `_totalWeight` must remain synchronized if an allocation or value construction throws.

A suitable insertion sequence is:

1. Validate the weight and calculate `newTotalWeight`.
2. Insert the entry.
3. Attempt to insert `newTotalWeight` into `_cumulativeWeights`.
4. If the cumulative insertion throws, remove the newly inserted entry and rethrow.
5. Assign `_totalWeight = newTotalWeight`.

### Binary-search selection

Replace the linear loop in `pickTarget()` with `std::upper_bound`:

```cpp
const auto it = std::upper_bound(
	_cumulativeWeights.begin(),
	_cumulativeWeights.end(),
	p_target);
```

The selected entry index is:

```cpp
const std::size_t index = static_cast<std::size_t>(
	std::distance(_cumulativeWeights.begin(), it));
```

Return:

```cpp
return _entries[index].value;
```

Using `std::upper_bound` preserves the current half-open interval semantics.

For cumulative boundaries:

```text
2.0, 7.0, 10.0
```

the selections must be:

```text
[0.0, 2.0)  -> first entry
[2.0, 7.0)  -> second entry
[7.0, 10.0) -> third entry
```

A target exactly equal to an entry boundary must select the following entry.

The complexity of `pickTarget()` must be `O(log n)`.

### Unit-roll selection

Keep:

```cpp
[[nodiscard]]
const TValue &pick(double p_unitRoll) const;
```

It must accept values in:

```text
[0, 1)
```

Calculate:

```cpp
double target = p_unitRoll * _totalWeight;
```

Protect against floating-point rounding producing a target equal to `_totalWeight`:

```cpp
if (target >= _totalWeight)
{
	target = std::nextafter(_totalWeight, 0.0);
}
```

Then call `pickTarget(target)`.

An empty pool must still produce a clear `std::logic_error`.

### Additional API improvements

Add:

```cpp
void clear() noexcept;
```

It must clear both vectors and reset `_totalWeight`:

```cpp
_entries.clear();
_cumulativeWeights.clear();
_totalWeight = 0.0;
```

Keep:

```cpp
reserve();
empty();
size();
totalWeight();
front();
back();
begin();
end();
```

Update `reserve()` so it reserves both internal vectors:

```cpp
void reserve(std::size_t p_count)
{
	_entries.reserve(p_count);
	_cumulativeWeights.reserve(p_count);
}
```

### Emplacement support

Add an emplacement method for expensive or move-only values:

```cpp
template <typename... TArguments>
TValue &emplace(double p_weight, TArguments &&...p_arguments);
```

It must perform the same weight validation and cumulative-weight updates as `add()`.

Expected usage:

```cpp
pool.emplace(
	5.0,
	constructorArgumentA,
	constructorArgumentB);
```

Avoid duplicating validation and insertion logic between `add()` and `emplace()`. Introduce private helpers where useful.

### Initializer-list constructor

Keep the initializer-list constructor, but explicitly constrain it to copy-constructible values:

```cpp
WeightedPool(std::initializer_list<Entry> p_entries)
	requires std::copy_constructible<TValue>;
```

Include `<concepts>`.

Move-only values should remain supported through `add()` and `emplace()`, but not through `std::initializer_list`.

### Required includes

Add the headers required by the new implementation:

```cpp
#include <algorithm>
#include <concepts>
#include <iterator>
```

Keep the existing mathematical and container headers.

### Expected internal invariants

After every successful public operation:

```cpp
_entries.size() == _cumulativeWeights.size()
```

When the pool is not empty:

```cpp
_cumulativeWeights.back() == _totalWeight
```

Every cumulative value must be finite, positive, and strictly greater than the preceding value.

When the pool is empty:

```cpp
_totalWeight == 0.0
_cumulativeWeights.empty()
```

### Tests

Add tests covering:

* Adding valid weighted entries.
* Rejection of zero, negative, NaN, and infinite weights.
* Rejection of total-weight overflow.
* Rejection of weights lost through floating-point precision.
* Correct cumulative-weight construction.
* Selection inside every weighted interval.
* Exact-boundary selection chooses the following entry.
* A target immediately below a boundary chooses the preceding entry.
* `pick(0.0)` selects the first entry.
* Unit rolls close to `1.0` select the final entry without producing an invalid target.
* Invalid unit rolls are rejected.
* Invalid direct targets are rejected.
* Empty-pool selection is rejected.
* `clear()` resets all state.
* `reserve()` reserves both collections without changing their size.
* `emplace()` constructs and returns the inserted value.
* Move-only values work through `add()` and `emplace()`.
* Large pools return the same selections as a reference linear implementation.

Preserve the separation between weighted selection and random-number generation. The caller must continue providing either a normalized roll or a direct target.
