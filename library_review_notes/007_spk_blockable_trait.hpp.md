# Review 007 — `includes/structures/design_pattern/spk_blockable_trait.hpp`

- File: [includes/structures/design_pattern/spk_blockable_trait.hpp](<../includes/structures/design_pattern/spk_blockable_trait.hpp>)
- Done [X]
- Remarks and requested changes:

Refactor `spk::BlockableTrait` to clarify its blocking semantics, support multiple deferred operations, improve RAII safety, and make the shared state thread-safe.

Preserve the existing design based on a shared internal `State` and move-only `Blocker` RAII handles.

## Blocking modes

Rename:

```cpp
Mode::Delay
```

to:

```cpp
Mode::Defer
```

The final modes should be:

```cpp
enum class Mode
{
	Ignore,
	Defer
};
```

Their semantics are:

* `Ignore`: operations submitted while this mode is active are discarded.
* `Defer`: operations submitted while this mode is active are stored and executed later.
* When both modes are active, `Ignore` takes precedence for newly submitted operations.
* Deferred operations are executed only when every `Ignore` and `Defer` blocker has been released.

Rename related members accordingly:

```cpp
nbDelayBlocks -> nbDeferBlocks
isDelayBlocked() -> isDeferBlocked()
```

## Operation type

Since the project uses C++23, replace `std::function<void()>` with:

```cpp
using Operation = std::move_only_function<void() noexcept>;
```

Operations must be non-throwing because they may be executed as part of an RAII blocker release.

This also allows operations containing move-only captures.

## Deferred operation queue

The current implementation stores only one delayed operation:

```cpp
std::function<void()> delayedOperation;
```

Replace it with a FIFO collection containing every deferred operation:

```cpp
std::deque<Operation> deferredOperations;
```

Include:

```cpp
#include <deque>
```

When several operations are submitted during a defer block:

```cpp
_executeOrBlock(operationA);
_executeOrBlock(operationB);
_executeOrBlock(operationC);
```

they must all be retained and later executed in insertion order:

```text
operationA
operationB
operationC
```

No operation should replace or merge with a previously deferred operation.

## Centralized operation handling

Replace the protected `_deferUntilUnblocked()` method with:

```cpp
void _executeOrBlock(Operation p_operation);
```

Its behavior must be:

1. Reject an empty operation if necessary.
2. Lock the shared state.
3. If at least one `Ignore` blocker exists, discard the operation.
4. Otherwise, if at least one `Defer` blocker exists, append the operation to `deferredOperations`.
5. Otherwise, unlock the state and execute the operation immediately.

Never execute an operation while holding the state mutex.

Expected usage from derived classes:

```cpp
_executeOrBlock(
	[this]()
	{
		_updateInternalState();
	});
```

Derived classes should no longer need to manually inspect the active blocking modes.

## Thread safety

Add a mutex to `State`:

```cpp
mutable std::mutex mutex;
```

Protect all accesses to:

```cpp
nbIgnoreBlocks
nbDeferBlocks
deferredOperations
```

The following operations must be safe when used concurrently:

* Creating blockers.
* Explicitly releasing blockers.
* Destroying blockers.
* Querying the blocking state.
* Submitting operations through `_executeOrBlock()`.

Never invoke user-provided operations while holding the mutex.

When the final blocker is released:

1. Lock the state.
2. Decrement the corresponding block counter.
3. If both counters are now zero, move all deferred operations into a local container.
4. Clear the shared deferred queue.
5. Unlock the state.
6. Execute the local operation batch in FIFO order.

The batch moved out when the state becomes unblocked should execute completely. Operations submitted during its execution should follow the current blocking state normally.

## Blocker release semantics

The public explicit release operation must be checked:

```cpp
void release();
```

Calling it on an invalid, already released, default-constructed, or moved-from blocker must throw `std::logic_error`.

Expected behavior:

```cpp
auto blocker = object.block();

blocker.release();
blocker.release(); // Throws std::logic_error.
```

However, destruction and move assignment must remain non-throwing.

Add a private release helper:

```cpp
void _release(bool p_throwIfInvalid);
```

Use it as follows:

```cpp
void release()
{
	_release(true);
}

~Blocker() noexcept
{
	_release(false);
}
```

The helper must:

* Lock the weak state.
* Detect whether the blocker still owns a block.
* Throw only when `p_throwIfInvalid` is `true`.
* Invalidate the blocker before running deferred operations.
* Decrement exactly one block counter.
* Detect counter underflow as an internal logic error where possible.
* Move pending operations out of the state when all blockers are gone.
* Execute deferred operations outside the mutex.

The destructor must never allow an exception to escape.

## Blocker move semantics

Keep `Blocker` move-only.

The move constructor must transfer ownership of the active block and invalidate the source blocker.

The move-assignment operator must:

1. Silently release the destination's currently owned block using `_release(false)`.
2. Transfer the source state and mode.
3. Invalidate the source.
4. Remain `noexcept`.

Self-move assignment must be handled safely.

A moved-from blocker must report itself as invalid, and an explicit call to `release()` on it must throw.

## Blocker construction

Make the constructor receiving the internal state private:

```cpp
explicit Blocker(
	const std::shared_ptr<State> &p_state,
	Mode p_mode);
```

Declare `BlockableTrait` as a friend:

```cpp
friend class BlockableTrait;
```

External code should only obtain blockers through:

```cpp
block();
```

## BlockableTrait copy and move behavior

Explicitly delete copying and moving:

```cpp
BlockableTrait(const BlockableTrait &) = delete;
BlockableTrait &operator=(const BlockableTrait &) = delete;

BlockableTrait(BlockableTrait &&) = delete;
BlockableTrait &operator=(BlockableTrait &&) = delete;
```

Copying the shared pointer would unexpectedly cause multiple objects to share the same blocking state.

Moving could also invalidate deferred operations that capture the original object through `this`.

## `block()` behavior

Mark `block()` as `[[nodiscard]]` because discarding the returned temporary immediately releases the block:

```cpp
[[nodiscard("The returned Blocker must remain alive")]
Blocker block(Mode p_mode = Mode::Ignore);
```

Creating a blocker must:

1. Lock the shared state.
2. Increment the counter corresponding to the selected mode.
3. Return an active `Blocker`.

Check for counter overflow before incrementing.

## Query methods

Expose:

```cpp
[[nodiscard]]
bool isBlocked() const noexcept;

[[nodiscard]]
bool isIgnoreBlocked() const noexcept;

[[nodiscard]]
bool isDeferBlocked() const noexcept;
```

Each query must lock the state mutex before reading the counters.

Use `std::size_t` consistently for block counters:

```cpp
std::size_t nbIgnoreBlocks = 0;
std::size_t nbDeferBlocks = 0;
```

## Expected interface

The final interface should approximately resemble:

```cpp
class BlockableTrait
{
public:
	enum class Mode
	{
		Ignore,
		Defer
	};

protected:
	using Operation =
		std::move_only_function<void() noexcept>;

private:
	struct State
	{
		mutable std::mutex mutex;

		std::size_t nbIgnoreBlocks = 0;
		std::size_t nbDeferBlocks = 0;

		std::deque<Operation> deferredOperations;
	};

	std::shared_ptr<State> _state =
		std::make_shared<State>();

protected:
	BlockableTrait();

	virtual ~BlockableTrait();

	BlockableTrait(const BlockableTrait &) = delete;
	BlockableTrait &operator=(const BlockableTrait &) = delete;

	BlockableTrait(BlockableTrait &&) = delete;
	BlockableTrait &operator=(BlockableTrait &&) = delete;

	void _executeOrBlock(Operation p_operation);

public:
	class Blocker
	{
		friend class BlockableTrait;

	private:
		std::weak_ptr<State> _state;
		Mode _mode = Mode::Ignore;

		explicit Blocker(
			const std::shared_ptr<State> &p_state,
			Mode p_mode);

		void _release(bool p_throwIfInvalid);

	public:
		Blocker() = default;
		~Blocker() noexcept;

		Blocker(const Blocker &) = delete;
		Blocker &operator=(const Blocker &) = delete;

		Blocker(Blocker &&p_other) noexcept;
		Blocker &operator=(Blocker &&p_other) noexcept;

		void release();

		[[nodiscard]]
		bool isValid() const noexcept;
	};

	[[nodiscard("The returned Blocker must remain alive")]]
	Blocker block(Mode p_mode = Mode::Ignore);

	[[nodiscard]]
	bool isBlocked() const noexcept;

	[[nodiscard]]
	bool isIgnoreBlocked() const noexcept;

	[[nodiscard]]
	bool isDeferBlocked() const noexcept;
};
```

## Tests

Add tests covering:

* A default-constructed blocker is invalid.
* An active blocker is valid.
* Destroying a blocker releases its block.
* Explicit release invalidates the blocker.
* Releasing the same blocker twice throws.
* Releasing a default-constructed blocker throws.
* Releasing a moved-from blocker throws.
* Move construction transfers block ownership.
* Move assignment releases the destination's previous block before transferring ownership.
* Nested ignore blockers update their counter correctly.
* Nested defer blockers update their counter correctly.
* Operations execute immediately when unblocked.
* Operations are discarded while ignore-blocked.
* Operations are queued while defer-blocked.
* Multiple deferred operations all execute in FIFO order.
* Deferred operations do not execute until every defer blocker is gone.
* Deferred operations do not execute while an ignore blocker remains active.
* Ignore takes precedence over defer for newly submitted operations.
* Deferred operations execute outside the mutex.
* A blocker may safely outlive the `BlockableTrait`.
* Concurrent blocker creation, release, state queries, and deferred-operation submission do not produce data races.
* Counter overflow is detected.
