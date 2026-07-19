# Review 004 — `includes/structures/container/spk_object_pool.hpp`

- File: [includes/structures/container/spk_object_pool.hpp](<../includes/structures/container/spk_object_pool.hpp>)
- Done [X]
- Remarks and requested changes:

Refactor `spk::ObjectPool<TType>` to simplify its API, improve exception safety, and make it thread-safe.

Apply the following design changes.

### Public API

Use this terminology:

```cpp
pool.acquire();                 // Obtain an object.
pool.preallocate(32);           // Create cached objects in advance.
pool.purge();                   // Destroy every currently cached object.
pool.nbAvailableObject();       // Number of objects currently cached.
pool.setMaximumCachedObjectCount(128);
pool.close();                   // Permanently close the pool.
```

Rename:

* `obtain()` to `acquire()`.
* `reserve()` to `preallocate()`.
* `release()` to `purge()`.
* `size()` to `nbAvailableObject()`.

Keep:

```cpp
empty();
isClosed();
close();
```

### Immutable configuration

Remove the default constructor and the `configure()` method.

The generator must be provided during construction:

```cpp
explicit ObjectPool(
	Generator p_generator,
	OnObtain p_onObtain = nullptr);
```

The configuration must remain immutable for the complete lifetime of the pool. This prevents objects acquired under one configuration from returning after the pool has been reconfigured.

Validate during construction that the generator is not empty.

### Callback types

Since the project uses C++23, replace `std::function` with `std::move_only_function`:

```cpp
using Generator =
	std::move_only_function<std::unique_ptr<TType>()>;

using OnObtain =
	std::move_only_function<void(TType &)>;
```

This allows the pool to own callbacks containing move-only captures.

### Exception-safe acquisition

Remove `_obtainRawPointer()`.

During `acquire()`, keep the acquired object inside a local `std::unique_ptr<TType>` until:

* It has been created or extracted from the cache.
* The generator result has been validated.
* `OnObtain` has completed successfully.
* The final pooled handle is constructed.

Example logic:

```cpp
std::unique_ptr<TType> object;

if (cached objects are available)
{
	object = std::move(last cached object);
	remove it from the cache;
}
else
{
	object = generator();

	if (object == nullptr)
	{
		throw std::logic_error(
			"spk::ObjectPool: generator returned nullptr");
	}
}

if (onObtain != nullptr)
{
	onObtain(*object);
}

return Handle(
	object.release(),
	ReturnToPoolDeleter(data));
```

If the generator or `OnObtain` throws, the local `unique_ptr` must safely destroy the object.

### Safe custom deleter

Make `ReturnToPoolDeleter::operator()` explicitly `noexcept`.

Immediately recover ownership of the raw pointer:

```cpp
void operator()(TType *p_ptr) const noexcept
{
	std::unique_ptr<TType> object(p_ptr);
	// ...
}
```

When the handle is destroyed:

* If the pointer is null, do nothing.
* If the shared pool data no longer exists, destroy the object.
* If the pool is closed, destroy the object.
* If the cache already reached its maximum size, destroy the object.
* Otherwise, return it to the cache.

If inserting the object into the cache throws, catch the exception and allow the local `unique_ptr` to destroy the object. No exception may escape from the deleter.

### `close()` semantics

Keep `close()` because it must allow the pool to be permanently disabled while some acquired handles may still exist.

Calling `close()` must:

* Mark the shared pool data as closed.
* Destroy every currently cached object.
* Prevent future calls to `acquire()`, `preallocate()`, `purge()`, and configuration-modifying methods.
* Ensure that objects returned by still-existing handles are destroyed instead of being inserted back into the pool.
* Be idempotent and `noexcept`.

The existing `weak_ptr<Data>` mechanism should continue to support handles surviving the destruction of the `ObjectPool` instance.

The destructor must call `close()`.

### Thread safety

Add a mutex inside the shared `Data` structure:

```cpp
mutable std::mutex mutex;
```

Protect all shared mutable state:

* `availableObjects`
* `maximumCachedObjectCount`
* `closed`

Ensure that:

* `acquire()` can safely run concurrently with handle destruction.
* Multiple handles can safely return objects concurrently.
* `purge()`, `preallocate()`, `setMaximumCachedObjectCount()`, and `close()` are synchronized.
* Read-only queries such as `empty()`, `nbAvailableObject()`, and `isClosed()` are synchronized.

Do not invoke user-provided callbacks while holding the mutex.

For `acquire()`:

1. Lock the mutex.
2. Validate that the pool is open.
3. Extract one cached object if available.
4. Unlock the mutex.
5. Invoke the generator if no cached object was found.
6. Invoke `OnObtain`.
7. Return the handle.

For `preallocate()`:

1. Determine how many objects are missing while locked.
2. Create objects outside the mutex.
3. Lock again and insert as many as remain useful.
4. Destroy excess generated objects if the pool was closed or its limit changed during generation.

For the custom deleter:

1. Lock the shared data.
2. Check `closed` and the current cache limit.
3. Insert the object safely.
4. Never allow exceptions to escape.

### Cache behavior

`preallocate(p_count)` represents the requested total number of cached objects, not the number of additional objects to create.

It must never exceed `maximumCachedObjectCount`.

Clamp the requested count:

```cpp
const std::size_t targetCount =
	std::min(p_count, maximumCachedObjectCount);
```

`setMaximumCachedObjectCount()` must immediately destroy excess cached objects when the new limit is lower than the current cache size.

`purge()` must only destroy cached objects. It must not close the pool and must not affect objects currently held by handles.

### Expected public interface

The final API should approximately expose:

```cpp
template <typename TType>
class ObjectPool
{
public:
	using Generator =
		std::move_only_function<std::unique_ptr<TType>()>;

	using OnObtain =
		std::move_only_function<void(TType &)>;

	using Handle =
		std::unique_ptr<TType, ReturnToPoolDeleter>;

	explicit ObjectPool(
		Generator p_generator,
		OnObtain p_onObtain = nullptr);

	~ObjectPool();

	ObjectPool(const ObjectPool &) = delete;
	ObjectPool &operator=(const ObjectPool &) = delete;

	ObjectPool(ObjectPool &&) = delete;
	ObjectPool &operator=(ObjectPool &&) = delete;

	[[nodiscard]]
	Handle acquire();

	void preallocate(std::size_t p_count);

	void purge();

	void setMaximumCachedObjectCount(std::size_t p_count);

	[[nodiscard]]
	std::size_t nbAvailableObject() const noexcept;

	[[nodiscard]]
	bool empty() const noexcept;

	[[nodiscard]]
	bool isClosed() const noexcept;

	void close() noexcept;
};
```

Preserve the existing lifetime model based on shared internal data and a `weak_ptr` stored in each handle deleter, while making every operation exception-safe and thread-safe.
