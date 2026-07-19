# Review 008 — `includes/structures/design_pattern/spk_inherence_trait.hpp`

- File: [includes/structures/design_pattern/spk_inherence_trait.hpp](<../includes/structures/design_pattern/spk_inherence_trait.hpp>)
- Done [X]
- Remarks and requested changes:

Refactor `spk::HierarchyTrait<TType>` to simplify its deferred-mutation system, strengthen hierarchy invariants, and make traversal and destruction safer.

Preserve the existing CRTP architecture and non-owning parent/child pointers. Do not convert the hierarchy to `shared_ptr` ownership.

## Threading policy

Keep `HierarchyTrait` explicitly single-threaded.

Do not add a mutex that would provide only partial protection around raw node pointers. Document that:

* A hierarchy and its traversal guards must be manipulated from the same thread.
* External synchronization is required when the hierarchy is shared between threads.
* `shared_ptr` and `weak_ptr` are used for lifetime tracking, not thread safety.

## Traversal guard API

Rename:

```cpp
HierarchyMutationGuard
```

to:

```cpp
ChildrenTraversalGuard
```

The guard protects the child collection of one hierarchy node while it is being traversed.

Make its state-based constructor private and expose a dedicated method:

```cpp
[[nodiscard("The returned guard must remain alive during traversal")]]
ChildrenTraversalGuard guardChildrenTraversal() const;
```

Expected usage:

```cpp
auto guard = node.guardChildrenTraversal();

for (TType *child : node.children())
{
	if (child != nullptr)
	{
		// Process child.
	}
}
```

Creating a guard must increment the owner's traversal-block counter and check for counter overflow.

## Strict guard release semantics

The explicit release operation must detect incorrect usage:

```cpp
void release();
```

Calling `release()` on an invalid, default-constructed, already released, or moved-from guard must throw `std::logic_error`.

Example:

```cpp
auto guard = node.guardChildrenTraversal();

guard.release();
guard.release(); // Throws.
```

The destructor and move assignment must remain non-throwing.

Use a private helper that distinguishes checked and unchecked release:

```cpp
void _release(bool p_throwIfInvalid);
```

Use it as follows:

```cpp
void release()
{
	_release(true);
}

~ChildrenTraversalGuard() noexcept
{
	_release(false);
}
```

The move constructor must transfer ownership and invalidate the source.

The move-assignment operator must:

1. Silently release the destination's current guard.
2. Transfer the source state.
3. Invalidate the source.
4. Remain `noexcept`.

If an unexpected exception occurs while the destructor flushes deferred hierarchy mutations, terminate rather than allowing an exception to escape from the destructor.

## Deferred mutation queue

Replace:

```cpp
std::vector<DeferredMutation> deferredMutations;
```

with a FIFO queue:

```cpp
std::deque<DeferredMutation> deferredMutations;
```

Include `<deque>`.

Remove the current optimization that replaces an existing `SetParent` mutation for the same node.

Every requested hierarchy mutation must be retained and executed in its original order. Coalescing mutations can change callback ordering and observable behavior.

For example:

```cpp
child.setParent(parentA);
child.setParent(parentB);
```

must execute both mutations in that order after traversal ends.

This preserves the semantics of every requested operation and the associated callbacks.

## Deferred mutation execution

When the final traversal guard is released:

1. Move the current deferred queue into a local container.
2. Mark the owner as unblocked.
3. Apply mutations in FIFO order.
4. Continue processing if callbacks or applied mutations schedule additional mutations.
5. Skip mutations whose target or requested parent has been destroyed.

A deferred `SetParent` may encounter another blocked parent when it is applied. In that case, it should be transferred to the other blocked hierarchy state and executed when that state becomes unblocked.

Do not recursively apply an unbounded call stack. Continue using an iterative flushing loop.

## Lifetime tokens

Replace the untyped token:

```cpp
std::shared_ptr<void> _aliveToken;
```

with a dedicated private type:

```cpp
struct LifetimeToken
{
};

std::shared_ptr<LifetimeToken> _aliveToken =
	std::make_shared<LifetimeToken>();
```

Deferred mutations should store:

```cpp
std::weak_ptr<LifetimeToken> nodeAliveToken;
std::weak_ptr<LifetimeToken> parentAliveToken;
```

Remove `hasParentAliveToken`.

The rules should be:

* A null requested parent means detachment and requires no parent token.
* A non-null requested parent must have a valid lifetime token.
* A mutation with an expired target token is discarded.
* A mutation with an expired non-null parent token is discarded.

## Safe hierarchy destruction

Do not invoke virtual hierarchy callbacks from the `HierarchyTrait` destructor.

At that point, the derived part of the node may already have been destroyed, so callbacks involving `TType*` can expose partially destroyed objects.

The destructor must perform silent structural cleanup:

* Invalidate `_aliveToken` first.
* Prevent the node's deferred mutations from being applied.
* Remove the node from its parent without invoking `_onChildRemoved()` or `_onParentChanged()`.
* Detach its children without invoking `_onParentChanged()` or `_onChildRemoved()`.
* Clear the traversal state's owner pointer.
* Remain `noexcept`.

Normal explicit hierarchy operations must continue invoking callbacks. Destructor cleanup must not.

## Destruction during child traversal

Deleting or destroying a child while its parent's child collection is guarded must not erase an element from the parent's vector because that would invalidate active iterators.

Use tombstone removal:

```cpp
*childIterator = nullptr;
```

instead of erasing the vector element while traversal is blocked.

Add a flag to `TraversalState`:

```cpp
bool requiresChildCompaction = false;
```

When a child is silently detached during a blocked traversal:

* Replace its pointer in the parent child vector with `nullptr`.
* Set `requiresChildCompaction` to `true`.
* Do not change the vector size.
* Do not invoke hierarchy callbacks.

When the final traversal guard is released:

1. Compact null child pointers.
2. Reset `requiresChildCompaction`.
3. Flush deferred hierarchy mutations.

Add a private helper:

```cpp
void _compactChildren() noexcept;
```

that removes all null pointers from `_children`.

When the hierarchy is not traversal-blocked, silent destruction cleanup may erase the pointer immediately.

The hierarchy must never contain null children once all traversal guards have been released.

## Safe child traversal helper

Add a protected or public helper for common traversal:

```cpp
template <typename TCallback>
void forEachChild(TCallback &&p_callback);
```

Its behavior must be:

1. Create a `ChildrenTraversalGuard`.
2. Iterate through `_children`.
3. Skip null tombstones.
4. Invoke the callback for each live child.

Example:

```cpp
node.forEachChild(
	[](TType &p_child)
	{
		p_child.update();
	});
```

Provide a const overload when useful.

This should be the preferred traversal API because it automatically applies the correct guard and skips destruction tombstones.

## Stronger reparenting safety

Improve `_setParentImmediate()` so an allocation failure while inserting into the new parent's child vector does not leave the node detached from its previous parent.

For a non-null new parent:

1. Validate the operation.
2. Insert the node into the new parent's child vector first.
3. Only after insertion succeeds, remove it from the old parent's child vector.
4. Update `_parent`.
5. Invoke callbacks after the structural invariants are fully established.

The callback order should be explicitly defined as:

1. Old parent receives `_onChildRemoved()`.
2. New parent receives `_onChildAdded()`.
3. The node receives `_onParentChanged()`.

Mark hierarchy callbacks `noexcept`:

```cpp
virtual void _onChildAdded(TType *p_child) noexcept;
virtual void _onChildRemoved(TType *p_child) noexcept;
virtual void _onParentChanged(
	TType *p_oldParent,
	TType *p_newParent) noexcept;
```

Derived implementations must not throw.

## Cycle validation

Correct the semantics of:

```cpp
isAncestorOf();
isDescendantOf();
```

A node must not be considered its own ancestor or descendant.

Expected behavior:

```cpp
node.isAncestorOf(&node) == false;
node.isDescendantOf(&node) == false;
```

Add a private cycle-check helper:

```cpp
[[nodiscard]]
bool _wouldCreateCycle(const TType *p_parent) const noexcept;
```

It should walk upward from the proposed parent and return `true` if the current node is encountered.

In `setParent()`:

* Setting the existing parent remains a no-op.
* Setting the node as its own parent must throw `std::invalid_argument`.
* Creating an indirect cycle must throw `std::invalid_argument`.

Do not silently ignore invalid cycle requests.

## Public mutation validation

Use clear behavior for invalid arguments:

```cpp
addChild(nullptr);
removeChild(nullptr);
```

should throw `std::invalid_argument`.

Calling `removeChild()` with a valid node that is not currently a direct child may remain a no-op.

Calling `clearChildren()` on an empty node remains a no-op.

## Accessors

Mark non-throwing accessors `noexcept`:

```cpp
[[nodiscard]] bool isRoot() const noexcept;
[[nodiscard]] bool isLeaf() const noexcept;
[[nodiscard]] TType *parent() noexcept;
[[nodiscard]] const TType *parent() const noexcept;
[[nodiscard]] bool hasParent() const noexcept;
[[nodiscard]] bool hasChild(const TType *p_child) const noexcept;
[[nodiscard]] std::size_t nbChildren() const noexcept;
```

Provide both const and non-const parent accessors.

Consider returning a span instead of exposing the vector type directly:

```cpp
[[nodiscard]]
std::span<TType *const> children() noexcept;

[[nodiscard]]
std::span<TType *const> children() const noexcept;
```

If this exact span signature is inconvenient for const-correctness, provide suitable mutable and const element types.

The public API must not permit callers to add, remove, or reorder child pointers directly.

While a traversal guard is active, the returned child range may contain null tombstones. `forEachChild()` must always hide this detail by skipping them.

## Copy and move behavior

Keep copy and move operations deleted:

```cpp
HierarchyTrait(const HierarchyTrait &) = delete;
HierarchyTrait &operator=(const HierarchyTrait &) = delete;

HierarchyTrait(HierarchyTrait &&) = delete;
HierarchyTrait &operator=(HierarchyTrait &&) = delete;
```

Moving a hierarchy node would invalidate all raw parent and child links.

## Backward compatibility

Keep the existing compatibility alias when it is still used:

```cpp
template <typename TType>
using InherenceTrait = HierarchyTrait<TType>;
```

Optionally mark it deprecated if `HierarchyTrait` is now the preferred name.

## Expected state structure

The internal traversal state should approximately resemble:

```cpp
struct TraversalState
{
	HierarchyTrait<TType> *owner = nullptr;

	std::size_t nbBlocks = 0;

	bool requiresChildCompaction = false;

	std::deque<DeferredMutation> deferredMutations;
};
```

## Tests

Add or update tests covering:

* Root and leaf state.
* Adding, removing, and clearing children.
* Reparenting between two parents.
* Parent and child callbacks execute in the defined order.
* Setting the existing parent is a no-op.
* Direct self-parenting throws.
* Indirect cycle creation throws.
* A node is not its own ancestor or descendant.
* Strict ancestor and descendant relationships work.
* Nested traversal guards defer mutations until the final guard is released.
* Deferred mutations execute in FIFO order.
* Multiple `SetParent` operations for the same node are not coalesced.
* A deferred mutation is skipped when its target has been destroyed.
* A deferred mutation is skipped when its non-null parent has been destroyed.
* A deferred mutation can move from one blocked parent state to another.
* Explicitly releasing a guard twice throws.
* Releasing a default or moved-from guard throws.
* Guard move construction transfers ownership.
* Guard move assignment releases its previous ownership.
* A traversal guard may safely outlive its hierarchy owner.
* Destroying a child during parent traversal does not invalidate iteration.
* Destroying a child during traversal creates a temporary tombstone.
* Tombstones are compacted after the final guard is released.
* `forEachChild()` skips tombstones.
* Destruction cleanup does not invoke virtual hierarchy callbacks.
* Destroying a parent silently detaches all surviving children.
* Destroying a child silently removes it from its surviving parent.
* Failed allocation while attaching to a new parent does not detach the node from its old parent where test infrastructure permits allocation-failure simulation.
* Deferred mutations scheduled by callbacks are processed without recursive stack growth.

Preserve the existing lightweight non-owning hierarchy model while making traversal mutation, destruction, callback behavior, and invalid-operation handling explicit and safe.
