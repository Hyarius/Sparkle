# Review 009 — `includes/structures/design_pattern/spk_synchronizable_trait.hpp`

- File: [includes/structures/design_pattern/spk_synchronizable_trait.hpp](<../includes/structures/design_pattern/spk_synchronizable_trait.hpp>)
- Done [X]
- Remarks and requested changes:

Refactor `spk::SynchronizableTrait` to make synchronization requests thread-safe, prevent concurrent synchronization executions, preserve requests made during synchronization, and clarify failure semantics.

Preserve the existing purpose of the trait:

* `requestSynchronization()` marks the object as requiring synchronization.
* `synchronize()` performs synchronization only when requested.
* `forceSynchronization()` performs synchronization regardless of the current request state.
* Derived classes implement the actual work through `_synchronize()`.

## Replace the boolean with an explicit state

The current implementation uses:

```cpp
mutable std::atomic_bool _needsSynchronization = false;
```

Replace it with an atomic state containing two independent flags:

```cpp
enum class State : std::uint8_t
{
	None = 0,
	Requested = 1 << 0,
	Synchronizing = 1 << 1
};

mutable std::atomic<std::uint8_t> _state = 0;
```

Alternatively, use a dedicated private bitmask representation if this integrates cleanly with `spk::Flags`.

The state must distinguish:

* No synchronization is required.
* Synchronization has been requested.
* A synchronization operation is currently executing.
* A new request was made while synchronization was already executing.

A request made during `_synchronize()` must not be lost.

## Request semantics

Implement:

```cpp
void requestSynchronization() const noexcept;
```

by atomically adding the `Requested` flag.

Expected behavior:

```cpp
object.requestSynchronization();
object.requestSynchronization();
```

must remain equivalent to one pending synchronization request.

Requests are coalesced. The trait does not count how many requests were made.

## Query semantics

Keep:

```cpp
[[nodiscard]]
bool needsSynchronization() const noexcept;
```

It must return whether the `Requested` flag is currently set.

Also add:

```cpp
[[nodiscard]]
bool isSynchronizing() const noexcept;
```

This reports whether `_synchronize()` is currently being executed.

Optionally add:

```cpp
[[nodiscard]]
bool isSynchronizationPending() const noexcept;
```

as an explicit alias for `needsSynchronization()` only if this improves readability in existing code.

## Synchronization ownership

Only one thread may execute `_synchronize()` on a given object at a time.

Before executing the callback, atomically acquire synchronization ownership using a compare-and-exchange loop.

For normal `synchronize()`:

* If no synchronization is requested, return without doing anything.
* If another thread is already synchronizing, return without executing `_synchronize()`.
* Otherwise:

  * Set `Synchronizing`.
  * Clear the `Requested` flag that caused this synchronization pass.
  * Execute `_synchronize()`.

Clearing `Requested` before calling `_synchronize()` is important.

If another thread calls:

```cpp
requestSynchronization();
```

while `_synchronize()` is running, it must set `Requested` again. That request must remain pending after the current synchronization finishes.

Expected sequence:

```text
Requested
→ synchronization starts and consumes Requested
→ another request arrives during synchronization
→ synchronization finishes
→ Requested remains set
```

A later call to `synchronize()` must execute another synchronization pass.

## Successful synchronization

When `_synchronize()` succeeds:

* Clear only the `Synchronizing` flag.
* Do not clear `Requested` again.
* Preserve any request that arrived while synchronization was running.

Do not automatically loop and synchronize repeatedly in one call.

Each call to `synchronize()` should perform at most one synchronization pass. This prevents an object that continuously requests synchronization from monopolizing the calling thread.

## Failed synchronization

If `_synchronize()` throws:

* Restore the `Requested` flag.
* Clear the `Synchronizing` flag.
* Rethrow the original exception.

The object must remain marked as needing synchronization so the operation can be retried later.

Use an internal RAII guard if useful to guarantee state restoration.

Remove the current protected helpers:

```cpp
_beginSynchronization();
_failSynchronization();
```

The complete synchronization-state transition should be handled privately inside `synchronize()` and `forceSynchronization()`.

Derived classes should only implement:

```cpp
virtual void _synchronize() const = 0;
```

They should not manipulate synchronization state directly.

## Forced synchronization

Keep:

```cpp
void forceSynchronization() const;
```

Its semantics must be:

* Execute `_synchronize()` even if `Requested` is not currently set.
* Consume any request already pending when the forced synchronization begins.
* Preserve any new request made while `_synchronize()` is running.
* Prevent concurrent execution with another synchronization operation.
* Restore `Requested` if `_synchronize()` throws.

If another synchronization is already running, `forceSynchronization()` must not execute `_synchronize()` concurrently.

Because a `void` method cannot communicate that another synchronization currently owns the operation, choose and document one of these behaviors:

### Preferred behavior

Throw `std::logic_error` when `forceSynchronization()` is called while another synchronization is already running.

This makes failure to honor the word “force” explicit.

Normal `synchronize()` may remain a silent no-op when another synchronization is already running.

## Return-value improvement

Consider changing:

```cpp
void synchronize() const;
```

to:

```cpp
[[nodiscard]]
bool synchronize() const;
```

The return value should indicate whether `_synchronize()` was actually executed.

Return:

* `true` when one synchronization pass was executed successfully.
* `false` when no request existed or another synchronization was already running.

Similarly, consider:

```cpp
[[nodiscard]]
bool forceSynchronization() const;
```

only if concurrent forced synchronization should return `false` instead of throwing.

Prefer either:

```cpp
bool forceSynchronization();
```

with a documented `false` result when busy,

or:

```cpp
void forceSynchronization();
```

with `std::logic_error` when busy.

Do not silently ignore a forced synchronization request without documenting it.

## Copy and move behavior

Keep copying deleted:

```cpp
SynchronizableTrait(const SynchronizableTrait &) = delete;
SynchronizableTrait &operator=(const SynchronizableTrait &) = delete;
```

Delete move construction and move assignment as well:

```cpp
SynchronizableTrait(SynchronizableTrait &&) = delete;
SynchronizableTrait &operator=(SynchronizableTrait &&) = delete;
```

The current move implementation transfers only the atomic request flag. It cannot safely transfer:

* A synchronization operation currently in progress.
* A request racing with the move.
* Derived synchronization state associated with the original object.
* Callbacks or external references tied to the original address.

A synchronization trait should therefore remain address-stable unless a complete and explicitly synchronized move protocol is designed.

## Destruction behavior

Keep the destructor protected.

Because the class is polymorphic, choose one explicit destruction policy:

### Preferred for a trait not deleted through base pointers

```cpp
~SynchronizableTrait() = default;
```

Keep it protected and non-virtual, making deletion through a `SynchronizableTrait*` impossible.

### Alternative

Use:

```cpp
virtual ~SynchronizableTrait() = default;
```

only if instances are intended to be destroyed polymorphically through this base type.

Do not add a virtual destructor automatically unless polymorphic deletion is required.

The owner must guarantee that no thread is executing or entering synchronization while the derived object is being destroyed.

Document this lifetime requirement.

## Memory ordering

Use explicit atomic memory ordering.

Recommended baseline:

* `requestSynchronization()` uses `fetch_or(..., std::memory_order_release)`.
* Queries use `load(std::memory_order_acquire)`.
* Synchronization ownership uses compare-and-exchange with acquire-release semantics.
* Completion uses `fetch_and(..., std::memory_order_release)`.

Correctness and readability take priority over aggressively relaxed memory ordering.

If explicit ordering makes the implementation difficult to audit, use the default sequentially consistent ordering instead.

## Suggested internal helpers

Private helpers may include:

```cpp
[[nodiscard]]
bool _tryBeginSynchronization(bool p_force) const noexcept;

void _completeSynchronization() const noexcept;

void _failSynchronization() const noexcept;
```

Their responsibilities should be:

* `_tryBeginSynchronization(false)`:

  * Require `Requested`.
  * Reject an existing `Synchronizing` state.
  * Atomically set `Synchronizing` and consume `Requested`.

* `_tryBeginSynchronization(true)`:

  * Ignore whether `Requested` is present.
  * Reject an existing `Synchronizing` state.
  * Atomically set `Synchronizing` and consume any existing request.

* `_completeSynchronization()`:

  * Clear only `Synchronizing`.

* `_failSynchronization()`:

  * Restore `Requested`.
  * Clear `Synchronizing`.

These helpers must remain private. Derived classes should not call them.

## Expected interface

The final interface should approximately resemble:

```cpp
#pragma once

#include <atomic>
#include <cstdint>

namespace spk
{
	class SynchronizableTrait
	{
	private:
		enum StateFlag : std::uint8_t
		{
			None = 0,
			Requested = 1 << 0,
			Synchronizing = 1 << 1
		};

		mutable std::atomic<std::uint8_t> _state = None;

		[[nodiscard]]
		bool _tryBeginSynchronization(bool p_force) const noexcept;

		void _completeSynchronization() const noexcept;
		void _failSynchronization() const noexcept;

	protected:
		SynchronizableTrait() = default;
		~SynchronizableTrait() = default;

		virtual void _synchronize() const = 0;

	public:
		SynchronizableTrait(const SynchronizableTrait &) = delete;
		SynchronizableTrait &operator=(const SynchronizableTrait &) = delete;

		SynchronizableTrait(SynchronizableTrait &&) = delete;
		SynchronizableTrait &operator=(SynchronizableTrait &&) = delete;

		void requestSynchronization() const noexcept;

		[[nodiscard]]
		bool needsSynchronization() const noexcept;

		[[nodiscard]]
		bool isSynchronizing() const noexcept;

		[[nodiscard]]
		bool synchronize() const;

		void forceSynchronization() const;
	};
}
```

The exact return type of `synchronize()` may remain `void` if compatibility is important, but returning `bool` is preferred because it tells the caller whether work was performed.

## Tests

Add tests covering:

* A newly created object does not need synchronization.
* `requestSynchronization()` marks the object as pending.
* Multiple requests are coalesced.
* `synchronize()` does nothing when no request exists.
* `synchronize()` executes exactly once when requested.
* A successful synchronization consumes the initial request.
* A request made during `_synchronize()` remains pending afterward.
* A second call processes the request made during the first synchronization.
* A throwing `_synchronize()` restores the pending request.
* The original synchronization exception is rethrown.
* `isSynchronizing()` is true only while `_synchronize()` is running.
* Two concurrent calls cannot execute `_synchronize()` simultaneously.
* A concurrent normal `synchronize()` call returns without executing when another thread owns synchronization.
* `forceSynchronization()` executes without a pending request.
* Forced synchronization consumes a request that existed when it started.
* A request made during forced synchronization remains pending.
* Forced synchronization failure restores the request.
* Calling `forceSynchronization()` while another synchronization is active follows the selected documented behavior.
* Synchronization performs at most one pass per public call.
* Copy and move construction are unavailable.

Preserve the lightweight atomic design while making synchronization ownership, concurrent requests, failures, and forced execution behavior explicit and reliable.
