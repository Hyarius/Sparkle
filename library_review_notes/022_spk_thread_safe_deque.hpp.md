# Review 022 — `spk_thread_safe_deque.hpp`

- File: locate the declaration of `spk::ThreadSafeDeque` and all of its call sites.
- Done [X]
- Remarks and requested changes:

Refactor the current container into a straightforward thread-safe FIFO queue that is almost as easy to use as a normal standard container.

The user of the class should not need to manually lock a mutex, reason about check-then-act races, or understand the synchronization strategy in order to use it correctly.

The intended experience is:

```cpp
queue.push(value);

if (std::optional<Value> value = queue.tryPop())
{
	use(*value);
}
```

The class must hide its synchronization details and make the safe operation the natural operation.

The existing type uses `std::deque` internally, but its public API only exposes queue operations:

```cpp
pushBack(...)
emplaceBack(...)
popFront(...)
drain()
```

The required abstraction is a dynamically growing concurrent FIFO queue:

* Producers append values at the back.
* Consumers atomically retrieve and remove values from the front.
* Storage grows as needed, limited by available memory.
* Retrieving from an empty queue fails immediately through `tryPop()`.
* Retrieved values are owned by the caller and can be processed without holding any queue lock.
* Values are returned in FIFO order.
* Normal use must not require any explicit synchronization outside the class.

Do not prescribe a mutex-based or lock-free implementation in advance.

Review the actual usage patterns and choose the simplest implementation that provides the required correctness and ownership semantics. A mutex-protected `std::deque` is acceptable when it is the clearest solution. A lock-free or reduced-contention design is also acceptable when it provides a meaningful benefit and can be implemented safely without disproportionate complexity.

If a lock-free implementation is selected, explicitly justify:

* The expected producer and consumer model, such as SPSC, MPSC, SPMC, or MPMC.
* Whether the queue is truly unbounded or only dynamically allocated.
* The memory-reclamation strategy.
* The progress guarantee.
* The additional constraints imposed on `TType`.
* Why the added complexity is preferable to the mutex-protected design.

Do not describe a design as lock-free unless every operation claimed to be lock-free satisfies the corresponding progress guarantee.

## Primary usability objective

The main objective is not to expose every possible concurrent-queue feature.

The main objective is to provide a container that feels as simple and natural as using a standard FIFO container while remaining safe in a multithreaded environment.

A caller should be able to:

```cpp
spk::ThreadSafeQueue<Message> messages;

messages.push(Message{...});

if (std::optional<Message> message = messages.tryPop())
{
	process(std::move(*message));
}
```

without:

* Locking an external mutex.
* Calling `empty()` before attempting to pop.
* Keeping a lock while processing the retrieved value.
* Receiving a reference into internal storage.
* Knowing whether the implementation uses a mutex, atomics, or another synchronization mechanism.
* Coordinating producers and consumers manually for normal queue operations.

Correct concurrent usage must follow naturally from the public API.

The class should remain useful in ordinary single-threaded code as well. Thread safety must not make simple queue operations awkward.

The queue is always FIFO. Make that guarantee explicit in the type name, method names, documentation, and tests.

## Inspect and update usages

Before editing the class, inspect every use of:

```cpp
spk::ThreadSafeDeque
```

Confirm that callers only require FIFO behavior.

Rename the type to:

```cpp
spk::ThreadSafeQueue
```

Rename the corresponding header and implementation references according to the repository naming conventions.

Update all includes, declarations, member names, tests, and call sites.

Keep `std::deque<TType>` as the internal storage. The internal container is an implementation detail and does not need to match the public type name.

Do not add `pushFront()`, `popBack()`, iterators, indexed access, or other deque-specific operations merely to justify the old name.

## Standard-container-like ergonomics

Use familiar container vocabulary where it remains safe:

```cpp
push(...)
emplace(...)
empty()
size()
clear()
```

Use a distinct name for the atomic retrieval operation:

```cpp
tryPop()
```

Do not copy the exact `std::queue::front()` plus `pop()` interface.

In a concurrent container, this sequence would be unsafe:

```cpp
TType value = queue.front();
queue.pop();
```

Another consumer could modify the queue between those operations.

Instead, retrieval and removal must be one operation:

```cpp
std::optional<TType> value = queue.tryPop();
```

This is the only intentional departure from ordinary queue ergonomics, and it exists to make the safe usage pattern the easiest one.

Do not require the user to provide an output object:

```cpp
TType value;
queue.popFront(value);
```

Returning `std::optional<TType>` is clearer, supports more value types, and directly represents the possibility that the queue is empty.

## Required concurrent behavior

The container must provide a thread-safe FIFO abstraction suitable for transferring ownership of values between threads.

The core public operations are:

```cpp
void push(TType p_value);

template <typename... TArguments>
void emplace(TArguments &&...p_arguments);

[[nodiscard]]
std::optional<TType> tryPop();

[[nodiscard]]
bool empty() const;

[[nodiscard]]
SizeType size() const;

void clear();
```

Keep `drain()` only when current call sites benefit from retrieving the complete queued batch at once.

`tryPop()` is the central operation. It must atomically:

1. Determine whether an element is available.
2. Select the oldest available element.
3. Remove that element from the queue.
4. Transfer the value to the caller.

Two successful consumers must never receive the same element.

A produced element must be observable at most once through successful retrieval.

The queue should grow dynamically and must not impose an arbitrary fixed capacity. Exhaustion of system memory is outside the normal queue contract.

The default retrieval operation must be non-blocking:

* `tryPop()` returns `std::nullopt` when no value is currently available.
* It must not wait for a future producer.
* It must not require callers to check `empty()` first.

A blocking retrieval operation may be proposed as an additional API only when existing call sites clearly need it. Do not replace `tryPop()` with a blocking-only interface.

The implementation strategy is intentionally open. Compare the reasonable alternatives before selecting one:

* A mutex-protected `std::deque`.
* A mutex-protected linked queue.
* A standard-library or existing project concurrent queue, when one is already available and appropriate.
* A specialized SPSC or MPSC queue when the call sites prove that narrower concurrency model.
* A fully concurrent lock-free queue when its complexity is justified.

Prefer the implementation that makes the class easiest to maintain and hardest to misuse while satisfying the demonstrated producer/consumer model and expected contention.

Ease of use and correctness are more important than whether the implementation is technically lock-free.

Regardless of the implementation, do not expose:

* References, pointers, or iterators into internal storage.
* The synchronization primitive itself.
* An API that separates the empty check from the removal operation.
* Arbitrary callbacks whose execution lifetime or synchronization behavior is unclear.

Document that:

* Concurrent calls to the supported queue operations are safe.
* Destruction of the queue must not race with another operation.
* `empty()` and `size()` are snapshots when those methods are provided.
* `tryPop()` is the atomic check-and-remove operation.
* The exact ordering guarantee between concurrent producers must be documented.

## Public type aliases

Add useful aliases:

```cpp
using ValueType = TType;
using SizeType = typename std::deque<TType>::size_type;
```

Use `SizeType` for the return value of `size()`.

Do not use an unqualified `size_t`.

## Push operations

Replace:

```cpp
void pushBack(TType p_value);
```

with:

```cpp
void push(TType p_value);
```

Keep the parameter passed by value.

This is intentional: construction or copying of the function argument occurs before the mutex is acquired, and only the final move into the queue occurs while the queue is locked.

Do not replace this overload with separate `const TType &` and `TType &&` overloads unless an existing repository requirement makes that necessary.

The implementation should follow this form:

```cpp
void push(TType p_value)
{
	std::lock_guard lock(_mutex);
	_values.push_back(std::move(p_value));
}
```

Preserve support for direct in-place construction through:

```cpp
template <typename... TArguments>
void emplace(TArguments &&...p_arguments);
```

The method must construct the new element at the back of the queue.

Do not return a reference from `emplace()`. Such a reference would become unsafe as soon as the mutex is released.

Document that `emplace()` necessarily constructs the element while the mutex is held. Callers that need expensive preparation outside the lock should construct the value first and call `push()`.

## Pop operation

Remove the output-parameter API:

```cpp
[[nodiscard]]
bool popFront(TType &p_output);
```

It unnecessarily requires the caller to provide an existing object and requires `TType` to be move-assignable.

Replace it with:

```cpp
[[nodiscard]]
std::optional<TType> tryPop();
```

Expected behavior:

* Return `std::nullopt` when the queue is empty.
* Otherwise, move-construct the result from the front element.
* Remove the front element only after the result has been constructed successfully.
* Preserve FIFO ordering.
* Do not default-construct or move-assign a `TType`.
* Support move-only, non-default-constructible, and non-move-assignable values when they are move-constructible.

Use a constraint when useful to provide a clear diagnostic:

```cpp
requires std::move_constructible<TType>
```

Do not name this method `pop()`, because a non-blocking operation that may fail should be explicit.


The caller must receive an independent value whose lifetime is no longer connected to the queue.

After `tryPop()` returns successfully, the caller must be able to perform slow or arbitrary processing on the returned object without blocking producers or other consumers through the queue's internal synchronization.

## Pop exception behavior

Do not remove the front element before constructing the returned value.

If construction of the extracted value throws, the queue must still contain the same number of elements.

The front value may be left in a moved-from state when `TType` has a throwing move constructor. This limitation should be documented as the normal basic guarantee for arbitrary movable types.

Do not attempt to hide this issue with unsafe manual storage or complicated rollback logic.

## Drain operation

First inspect whether call sites genuinely need bulk extraction.

When `drain()` is useful, it must atomically detach all elements that belong to the queue at its linearization point and return them in FIFO order.

For the current `std::deque`-based implementation, replace:

```cpp
[[nodiscard]]
std::vector<TType> drain();
```

with:

```cpp
[[nodiscard]]
std::deque<TType> drain();
```

A mutex-protected deque can implement this efficiently by swapping the internal container with an empty local deque:

```cpp
[[nodiscard]]
std::deque<TType> drain()
{
	std::deque<TType> result;

	{
		std::lock_guard lock(_mutex);
		result.swap(_values);
	}

	return result;
}
```

This avoids an extra allocation and an element-by-element move.

For another synchronization strategy, provide equivalent observable semantics without weakening correctness merely to preserve the method.

Requirements:

* Returned elements remain in FIFO order.
* Each detached element appears exactly once.
* The queue remains usable immediately after draining.
* Concurrent pushes are either part of the detached batch or remain in the queue according to a documented linearization point.
* Processing and destruction of returned values must not unnecessarily block unrelated queue operations.

Do not convert the result to a `std::vector` inside this class unless contiguous storage is an explicit caller requirement.

## Clear operation

`clear()` must atomically remove the elements that belong to the queue at its linearization point.

Avoid blocking unrelated queue operations while removed values execute potentially expensive destructors.

For a mutex-protected deque, detach the storage under the lock and destroy it afterward:

```cpp
void clear()
{
	std::deque<TType> values;

	{
		std::lock_guard lock(_mutex);
		values.swap(_values);
	}
}
```

For a lock-free or node-based implementation, apply the equivalent ownership rule safely through the selected reclamation strategy.

The queue must already appear empty to later operations while the detached values are being destroyed.

## Empty and size queries

Keep:

```cpp
[[nodiscard]]
bool empty() const;

[[nodiscard]]
SizeType size() const;
```

Each method must lock the mutex and return a snapshot.

Use direct boolean expressions:

```cpp
return _values.empty();
```

Do not write comparisons such as:

```cpp
_values.empty() == true
```

Do not add unlocked or approximate variants.

## Synchronization implementation

Do not force a particular synchronization primitive in the public contract.

When a mutex-protected implementation is retained:

* Keep critical sections as short as practical.
* Prefer a single mutex unless measurements or call-site constraints justify a more elaborate design.
* Use an RAII lock guard.
* Keep the mutex private.
* Do not mark locking operations `noexcept`.
* Move detached batches and expensive destruction outside the critical section.

When a lock-free implementation is selected:

* Do not implement an ad hoc algorithm without a clearly identified, proven design.
* Correctly handle node lifetime and reclamation.
* Account for the ABA problem where applicable.
* Specify the required atomic memory ordering.
* Avoid silently leaking retired nodes.
* Add stress tests and run them under ThreadSanitizer where supported, while recognizing that ThreadSanitizer does not prove lock-free correctness.
* Preserve support for the value categories actually required by the repository.

The public API must not expose whether the queue is mutex-based or lock-free, unless implementation-specific guarantees are deliberately part of the type's documented contract.

## Copy and move semantics

Keep copy construction and copy assignment deleted.

Keep move construction and move assignment deleted.

A queue contains a mutex and may already be concurrently referenced, so implicit movement of the synchronization boundary would be misleading and unsafe.

Do not implement custom move operations.

## Suggested public interface

Unless repository usage demonstrates a need for additional operations, the public interface should approximately become:

```cpp
template <typename TType>
class ThreadSafeQueue
{
public:
	using ValueType = TType;
	using SizeType = typename std::deque<TType>::size_type;

	ThreadSafeQueue() = default;

	ThreadSafeQueue(const ThreadSafeQueue &) = delete;
	ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

	ThreadSafeQueue(ThreadSafeQueue &&) = delete;
	ThreadSafeQueue &operator=(ThreadSafeQueue &&) = delete;

	void push(TType p_value);

	template <typename... TArguments>
	void emplace(TArguments &&...p_arguments);

	[[nodiscard]]
	std::optional<TType> tryPop()
		requires std::move_constructible<TType>;

	[[nodiscard]]
	std::deque<TType> drain();

	void clear();

	[[nodiscard]]
	bool empty() const;

	[[nodiscard]]
	SizeType size() const;
};
```

Minor syntax adjustments are allowed when required by the project's supported compiler, but preserve the API semantics described above.

## Includes

The declaration should approximately require:

```cpp
#include <concepts>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>
```

Remove:

```cpp
#include <cstddef>
#include <vector>
```

when they are no longer used.

Do not add broad convenience headers.

## Documentation

Add concise documentation explaining:

* The queue is FIFO.
* Operations are individually synchronized.
* The queue is non-blocking.
* `tryPop()` is the atomic check-and-remove operation.
* `empty()` and `size()` are snapshots only.
* Queue lifetime must be externally synchronized.
* No reference to an internally stored value escapes the mutex.
* `drain()` atomically detaches the current batch but does not prevent later pushes from occurring immediately afterward.

For `drain()`, make the linearization point clear: values pushed before the swap belong to the drained batch, while values pushed after the swap remain in the queue.

## Tests

Add deterministic unit tests covering:

* A newly constructed queue is empty.
* A newly constructed queue has size zero.
* `push()` inserts one value.
* `emplace()` constructs one value.
* Multiple values are returned in FIFO order.
* `tryPop()` returns `std::nullopt` for an empty queue.
* `tryPop()` decreases the queue size.
* The queue can be reused after becoming empty.
* A move-only value can be pushed and popped.
* A non-default-constructible value can be popped.
* A move-constructible but non-move-assignable value can be popped.
* `drain()` preserves FIFO order.
* `drain()` empties the queue.
* The queue can be reused after `drain()`.
* Draining an empty queue returns an empty deque.
* `clear()` empties the queue.
* The queue can be reused after `clear()`.
* `size()` and `empty()` remain internally consistent in single-threaded use.

Add concurrency tests covering:

* Multiple producer threads pushing disjoint value ranges.
* All produced values are eventually observable exactly once.
* One or more consumer threads repeatedly calling `tryPop()`.
* No value is duplicated.
* No value is lost.
* Concurrent pushes and a `drain()` operation leave every produced value either in the drained batch or in the queue.
* The queue remains valid after heavy concurrent use.

Do not write concurrency tests that depend on a particular thread scheduling order.

Use barriers, latches, atomics, or other deterministic synchronization primitives where needed.

When available in the project, run the tests under ThreadSanitizer.

## Destruction-outside-lock tests

Add a focused test demonstrating that `clear()` does not hold the queue mutex while removed values are destroyed.

Use a test value whose destructor blocks on explicit synchronization. While that destructor is blocked, another thread must still be able to call `push()` successfully.

Apply a similar test to values returned by `drain()` only when useful; the implementation should naturally release the mutex before the returned container is processed.

Avoid timing-only tests based solely on sleeps.

## Scope control

Do not add features merely because concurrent queues sometimes provide them.

Additional operations such as blocking retrieval, timed retrieval, shutdown, bulk insertion, or custom allocation should be introduced only when actual repository call sites need them.

A lock-free implementation is allowed, but lock freedom is not itself a requirement. Do not accept substantially greater complexity solely to advertise it.

The objective is the easiest robust implementation of this behavior:

```text
Use the container like a normal FIFO queue.
Push values without manually synchronizing.
Atomically retrieve and remove the oldest available value.
Process the retrieved value independently from the queue.
Allow concurrent producers and consumers without exposing synchronization details.
Grow storage as needed, limited by available memory.
```

The final class should feel like a standard queue adapted correctly for concurrency, not like a synchronization primitive that every caller must learn to coordinate manually.

Favor a small, unsurprising API over exposing implementation details.