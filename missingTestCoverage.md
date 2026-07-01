# Missing Test Coverage

The missing-point inventory was generated from the LLVM HTML report at `build/coverage/coverage-html/index.html` and its structured detail data at `build/coverage/coverage-summary.json` on 2026-06-30 at 13:29. The totals below were refreshed after the coverage work on 2026-06-30 at 17:34. The unchecked point-by-point inventory remains the original work queue, so some entries are now covered; use the refreshed HTML report for the authoritative current line view.

This inventory includes both incomplete file-level metrics and uncovered regions or branch outcomes inside individual template/inline-function instantiations. A file can therefore have a 100% aggregate summary and still appear below when the HTML detail view contains an uncovered (red) expansion.

Each checklist item identifies the source location, the missing test path, and, when applicable, the number of instrumented functions or instantiations affected.

## Coverage Totals

- Lines: 95.20% (18050/18960, missing 910).
- Branches: 85.83% (3878/4518, missing 640).
- Functions: 97.86% (2656/2714, missing 58).
- Regions: 95.24% (8110/8515, missing 405).
- Files with an incomplete aggregate metric: 99.
- Per-instantiation incomplete-file count: not recalculated for the retained baseline work queue.

## Coverage Work Completed

Tests added during this pass cover the following formerly missing areas:

- Animation controller selection, playback, switching, elapsed-time behavior, looping/non-looping updates, and animation-logic guard paths.
- `Component2D`, component containers/registries/stores, engine identity/null operations, and contract-provider exception/default/self-move behavior.
- `Entity2D`, `Transform2D` caching/hierarchy propagation, `Camera2D` projection/view/main-camera behavior, sprite-renderer state, sprite batching, camera UBO execution, and instanced sprite execution.
- Matrix identity/validation/inverse/stream overloads, unsigned vector instantiations, all event-view consumption instantiations, and thread-safe contract move assignment.
- `TextEdit`, `TextArea`, grid/form layouts, interface-window controls/input/padding, and slider edge cases.

## Unreachable Code Notes

The following defensive branches cannot be reached through the public API without memory corruption or undefined behavior. They should be considered for removal instead of being covered by tests.

- **Unreachable code:** `includes/structures/container/spk_object_pool.hpp:231` (`_data == nullptr`). Every constructor initializes `_data`, while copy and move construction/assignment are deleted. No supported operation can make the owning `shared_ptr` null.
- **Unreachable code:** `srcs/structures/system/time/spk_chronometer.cpp:72-75` (`_state != State::Running` in `_currentRunDuration`). The method is private and is called only after the caller has established the `Running` state: `elapsedTime()` calls it inside its running branch and `pause()` calls it after rejecting every non-running state.
- **Unreachable code:** null entries in `ContractProvider::_links` (`includes/structures/design_pattern/spk_contract_provider.hpp:150`, `:221`, `:239`, and `:254`). `subscribe()` is the only insertion path and always pushes a non-null `std::make_shared<Link>()`; contracts invalidate `Link::function` but never replace the stored `shared_ptr` with null.
- **Unreachable code:** `HierarchyTrait` null child entries (`includes/structures/design_pattern/spk_inherence_trait.hpp:257-261`). `addChild(nullptr)` returns before insertion, and `_setParentImmediate()` only inserts the non-null `self` pointer. The child vector is private and has no mutable accessor.
- **Unreachable code:** a null `_traversalState` on a live `HierarchyTrait` (`includes/structures/design_pattern/spk_inherence_trait.hpp:144-147` and `:176-179`). It is initialized at member construction and the type deletes copy and move operations; destruction does not reset it until the object itself is no longer callable.
- **Unreachable code:** a non-null deferred parent without an alive token (`includes/structures/design_pattern/spk_inherence_trait.hpp:119-122`). `_makeSetParentMutation()` is the only factory and always records the token and flag when its parent argument is non-null.
- **Unreachable code:** null component entries in `Entity::_components` (`srcs/structures/game_engine/spk_entity.cpp:83-86`). The vector is private and receives only a successfully created `std::make_unique<TComponent>`; removal erases the entry rather than resetting it.
- **Unreachable code:** null entity children in entity/transform propagation (`srcs/structures/game_engine/spk_entity.cpp:60`, `:106`, and `srcs/structures/game_engine/spk_transform_2d.cpp:20`). `HierarchyTrait` enforces the non-null child invariant described above.
- **Unreachable code:** `AnimationController2D::currentAnimation()` failing to find `_currentName` after `_hasCurrent` becomes true (`includes/structures/game_engine/spk_animation_2d.hpp:86`). `_hasCurrent` is set only after `play()` confirms that the name exists, and the class exposes no animation-removal operation.
- **Unreachable code:** the null branch of `Component::_attach` (`srcs/structures/game_engine/spk_component.cpp:21`). `_attach` is private to `Entity`; its only call site passes `this` from a live entity, while detachment uses the separate `_detach()` method.

## Missing Points

### includes/structures/container/spk_binary_field.hpp

Coverage: lines 94.87% (37/39, missing 2); branches 83.33% (5/6, missing 1); functions 100.00% (6/6, missing 0); regions 93.33% (14/15, missing 1).

Uncovered code regions:

- [ ] `includes/structures/container/spk_binary_field.hpp` - Lines 69-71 - Exercise this uncovered code path: `throw std::runtime_error("BinaryField assignment received a value with the wrong size.");`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/container/spk_binary_field.hpp` - Lines 111-113 - Exercise this uncovered code path: `throw std::runtime_error("BinaryField array assignment received values with the wrong size.");`.
- [ ] `includes/structures/container/spk_binary_field.hpp` - Lines 115-116 - Exercise this uncovered code path: `std::memcpy(data(), p_values.data(), sizeof(TValueType) * NSize); return *this;`.
- [ ] `includes/structures/container/spk_binary_field.hpp` - Lines 133-135 - Exercise this uncovered code path: `throw std::runtime_error("BinaryField::as received a type with the wrong size.");`. This region is uncovered in 2 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/container/spk_binary_field.hpp` - Line 68, cols 8-42 - Cover the true outcome in 3 instrumented instances of `sizeof(TValueType) != section.size`.
- [ ] `includes/structures/container/spk_binary_field.hpp` - Line 110, cols 8-50 - Cover the true outcome and false outcome of `sizeof(TValueType) * NSize != section.size`.
- [ ] `includes/structures/container/spk_binary_field.hpp` - Line 132, cols 8-42 - Cover the true outcome in 2 instrumented instances of `sizeof(TValueType) != section.size`.

### includes/structures/container/spk_cached_data.hpp

Coverage: lines 97.78% (88/90, missing 2); branches 100.00% (10/10, missing 0); functions 100.00% (19/19, missing 0); regions 100.00% (35/35, missing 0).

Missing functions or function instantiations:

- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 30-42 - Execute the currently uncalled function: `void _destroyData() const`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 45-57 - Execute the currently uncalled function: `void _generateData() const`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 63 - Execute the currently uncalled function: `explicit CachedData(Generator p_generator, Destructor p_destructor = nullptr) :`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 80-83 - Execute the currently uncalled function: `TType &get()`.

Uncovered code regions:

- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 32-34 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 36-42 - Exercise this uncovered code path: `if (_destructor != nullptr) _destructor(*_data); _data.reset();`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 36 - Exercise this uncovered code path: `_destructor != nullptr`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 37-39 - Exercise this uncovered code path: `_destructor(*_data);`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 51-57 - Exercise this uncovered code path: `if (_generator == nullptr) throw std::logic_error("spk::CachedData: generator not set"); _data = _generator();`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 51 - Exercise this uncovered code path: `_generator == nullptr`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 52-54 - Exercise this uncovered code path: `throw std::logic_error("spk::CachedData: generator not set");`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 56-57 - Exercise this uncovered code path: `_data = _generator();`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Lines 138-140 - Exercise this uncovered code path: `return std::nullopt;`. This region is uncovered in 2 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 31, cols 8-34 - Cover the true outcome in 3 instrumented instances and false outcome in 4 instrumented instances of `_data.has_value() == false`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 36, cols 8-30 - Cover the true outcome in 9 instrumented instances and false outcome in 4 instrumented instances of `_destructor != nullptr`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 46, cols 8-33 - Cover the false outcome of `_data.has_value() == true`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 51, cols 8-29 - Cover the true outcome in 7 instrumented instances and false outcome of `_generator == nullptr`.
- [ ] `includes/structures/container/spk_cached_data.hpp` - Line 137, cols 8-34 - Cover the true outcome in 2 instrumented instances of `_data.has_value() == false`.

### includes/structures/container/spk_grid_2d.hpp

Coverage: lines 100.00% (69/69, missing 0); branches 100.00% (8/8, missing 0); functions 100.00% (19/19, missing 0); regions 100.00% (27/27, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Uncovered code regions:

- [ ] `includes/structures/container/spk_grid_2d.hpp` - Lines 35-37 - Exercise this uncovered code path: `throw std::length_error("spk::Grid2D dimensions are too large");`.
- [ ] `includes/structures/container/spk_grid_2d.hpp` - Lines 88-90 - Exercise this uncovered code path: `throw std::out_of_range("spk::Grid2D coordinates are out of range");`.

Uncovered branch outcomes:

- [ ] `includes/structures/container/spk_grid_2d.hpp` - Line 34, cols 8-36 - Cover the true outcome of `count > Storage().max_size()`.
- [ ] `includes/structures/container/spk_grid_2d.hpp` - Line 82, cols 11-24 - Cover the false outcome of `p_x < width()`.
- [ ] `includes/structures/container/spk_grid_2d.hpp` - Line 82, cols 28-42 - Cover the false outcome of `p_y < height()`.
- [ ] `includes/structures/container/spk_grid_2d.hpp` - Line 87, cols 8-35 - Cover the true outcome of `contains(p_x, p_y) == false`.

### includes/structures/container/spk_object_pool.hpp

Coverage: lines 100.00% (135/135, missing 0); branches 97.37% (37/38, missing 1); functions 100.00% (18/18, missing 0); regions 100.00% (74/74, missing 0).

Uncovered branch outcomes:

- [ ] `includes/structures/container/spk_object_pool.hpp` - Line 231, cols 8-24 - Cover the true outcome of `_data == nullptr`.

### includes/structures/container/spk_observable_value.hpp

Coverage: lines 100.00% (105/105, missing 0); branches 100.00% (2/2, missing 0); functions 100.00% (26/26, missing 0); regions 100.00% (30/30, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Missing functions or function instantiations:

- [ ] `includes/structures/container/spk_observable_value.hpp` - Lines 34-42 - Execute the currently uncalled function: `ObservableValue &_assign(TType &&p_value)`.
- [ ] `includes/structures/container/spk_observable_value.hpp` - Lines 79-83 - Execute the currently uncalled function: `ObservableValue &operator=(const TOther &p_value)`.

Uncovered branch outcomes:

- [ ] `includes/structures/container/spk_observable_value.hpp` - Line 35, cols 8-25 - Cover the true outcome and false outcome in 5 instrumented instances of `_value != p_value`.

### includes/structures/design_pattern/spk_contract_provider.hpp

Coverage: lines 95.80% (137/143, missing 6); branches 83.33% (35/42, missing 7); functions 100.00% (22/22, missing 0); regions 97.06% (66/68, missing 2).

Missing functions or function instantiations:

- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 38 - Execute the currently uncalled function: `_flag(p_flag)`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 44-46 - Execute the currently uncalled function: `~TriggerGuard()`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 67 - Execute the currently uncalled function: `_link(p_link)`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 104-110 - Execute the currently uncalled function: `void resign()`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 139-153 - Execute the currently uncalled function: `void _cleanup()`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 149-151 - Execute the currently uncalled function: `[](const std::shared_ptr<Link> &p_link) {`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 156-170 - Execute the currently uncalled function: `void _triggerDeferred()`. All 19 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 166-168 - Execute the currently uncalled function: `[this](auto &&...p_arguments) {`. All 19 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 173 - Execute the currently uncalled function: `ContractProvider() = default;`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 174 - Execute the currently uncalled function: `~ContractProvider() override = default;`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 183-190 - Execute the currently uncalled function: `Contract subscribe(Callback p_callback)`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 193-233 - Execute the currently uncalled function: `void trigger(TArguments... p_arguments)`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 199-201 - Execute the currently uncalled function: `_deferUntilUnblocked([this]() {`. All 19 instrumented functions/instantiations at this location are unexecuted.

Uncovered code regions:

- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 119 - Exercise this uncovered code path: `_link->function != nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 125-127 - Exercise this uncovered code path: `return Blocker();`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 141-143 - Exercise this uncovered code path: `return;`. This region is uncovered in 19 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 158-160 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 195-205 - Exercise this uncovered code path: `if (isDelayBlocked()) _lastTriggerArguments.emplace(p_arguments...); _deferUntilUnblocked([this]() {`. This region is uncovered in 17 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 196 - Exercise this uncovered code path: `isDelayBlocked()`. This region is uncovered in 17 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 197-202 - Exercise this uncovered code path: `_lastTriggerArguments.emplace(p_arguments...); _deferUntilUnblocked([this]() { _triggerDeferred();`. This region is uncovered in 17 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 208-210 - Exercise this uncovered code path: `return;`. This region is uncovered in 17 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 217 - Exercise this uncovered code path: `++i`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 218-229 - Exercise this uncovered code path: `std::shared_ptr<Link> element = _links[i]; if (element == nullptr || element->function == nullptr ||`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 221 - Exercise this uncovered code path: `element == nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 221-222 - Exercise this uncovered code path: `if (element == nullptr || element->function == nullptr ||`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 221-223 - Exercise this uncovered code path: `if (element == nullptr || element->function == nullptr || element->isBlocked())`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 222 - Exercise this uncovered code path: `element->function == nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 223 - Exercise this uncovered code path: `element->isBlocked()`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 224-226 - Exercise this uncovered code path: `continue;`. This region is uncovered in 16 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 228-229 - Exercise this uncovered code path: `element->function(p_arguments...);`.

Uncovered branch outcomes:

- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 94, cols 9-25 - Cover the false outcome in 12 instrumented instances of `this != &p_other`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 105, cols 9-25 - Cover the false outcome in 6 instrumented instances of `_link != nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 119, cols 13-29 - Cover the true outcome and false outcome in 5 instrumented instances of `_link != nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 119, cols 33-59 - Cover the true outcome and false outcome in 6 instrumented instances of `_link->function != nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 124, cols 9-25 - Cover the true outcome of `_link == nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 140, cols 8-21 - Cover the true outcome in 21 instrumented instances and false outcome in 2 instrumented instances of `_isTriggering`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 150, cols 15-32 - Cover the true outcome in 21 instrumented instances and false outcome in 3 instrumented instances of `p_link == nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 150, cols 36-63 - Cover the true outcome in 18 instrumented instances and false outcome in 3 instrumented instances of `p_link->function == nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 157, cols 8-50 - Cover the true outcome in 21 instrumented instances and false outcome in 19 instrumented instances of `_lastTriggerArguments.has_value() == false`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 194, cols 8-19 - Cover the true outcome in 19 instrumented instances and false outcome in 2 instrumented instances of `isBlocked()`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 196, cols 9-25 - Cover the true outcome in 19 instrumented instances and false outcome in 19 instrumented instances of `isDelayBlocked()`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 207, cols 8-21 - Cover the true outcome in 19 instrumented instances and false outcome in 2 instrumented instances of `_isTriggering`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 217, cols 24-40 - Cover the true outcome in 3 instrumented instances and false outcome in 2 instrumented instances of `i < initialCount`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 221, cols 10-28 - Cover the true outcome in 21 instrumented instances and false outcome in 3 instrumented instances of `element == nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 222, cols 7-35 - Cover the true outcome in 19 instrumented instances and false outcome in 3 instrumented instances of `element->function == nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 223, cols 7-27 - Cover the true outcome in 19 instrumented instances and false outcome in 3 instrumented instances of `element->isBlocked()`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 239, cols 9-27 - Cover the false outcome of `element != nullptr`.
- [ ] `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 254, cols 9-27 - Cover the false outcome of `element != nullptr`.

### includes/structures/design_pattern/spk_inherence_trait.hpp

Coverage: lines 93.91% (293/312, missing 19); branches 80.77% (84/104, missing 20); functions 100.00% (37/37, missing 0); regions 96.15% (150/156, missing 6).

Missing functions or function instantiations:

- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 61-79 - Execute the currently uncalled function: `[[nodiscard]] static DeferredMutation _makeSetParentMutation(TType *p_node, TType *p_parent)`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 82-93 - Execute the currently uncalled function: `[[nodiscard]] static DeferredMutation _makeClearChildrenMutation(TType *p_node)`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 142-166 - Execute the currently uncalled function: `void _deferMutation(DeferredMutation p_mutation)`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 153-156 - Execute the currently uncalled function: `[&](const DeferredMutation &p_existingMutation) {`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 369-370 - Execute the currently uncalled function: `virtual void _onChildAdded(TType *p_child)`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 465-474 - Execute the currently uncalled function: `void clearChildren()`.

Uncovered code regions:

- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 98-100 - Exercise this uncovered code path: `return false;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 103-105 - Exercise this uncovered code path: `return false;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 113-115 - Exercise this uncovered code path: `return true;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 118-120 - Exercise this uncovered code path: `return false;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 138 - Exercise this uncovered code path: `trait`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 144-146 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 159-162 - Exercise this uncovered code path: `*iterator = std::move(p_mutation); return;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 171-173 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 183-185 - Exercise this uncovered code path: `continue;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 193-195 - Exercise this uncovered code path: `case DeferredMutationType::ClearChildren: _trait(mutation.node)->clearChildren(); break;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 206-208 - Exercise this uncovered code path: `return;`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 216-218 - Exercise this uncovered code path: `return;`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 247-257 - Exercise this uncovered code path: `TType *child = _children.back(); if (child == nullptr) _children.pop_back();`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 250 - Exercise this uncovered code path: `child == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 251-254 - Exercise this uncovered code path: `_children.pop_back(); continue;`. This region is uncovered in 4 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 256-257 - Exercise this uncovered code path: `_trait(child)->_setParentImmediate(nullptr);`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 270-272 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 311-313 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 402-404 - Exercise this uncovered code path: `return true;`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 431-434 - Exercise this uncovered code path: `blockedOwner->_deferMutation(_makeSetParentMutation(static_cast<TType *>(this), p_parent)); return;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 442-444 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 452-454 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 457-459 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 57, cols 12-38 - Cover the false outcome in 4 instrumented instances of `_traversalState != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 57, cols 42-72 - Cover the true outcome in 2 instrumented instances of `_traversalState->nbBlocks != 0`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 67, cols 8-25 - Cover the true outcome in 2 instrumented instances and false outcome in 4 instrumented instances of `p_node != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 72, cols 8-27 - Cover the true outcome in 2 instrumented instances and false outcome in 3 instrumented instances of `p_parent != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 87, cols 8-25 - Cover the true outcome and false outcome in 2 instrumented instances of `p_node != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 97, cols 8-34 - Cover the true outcome in 2 instrumented instances of `p_mutation.node == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 102, cols 8-51 - Cover the true outcome of `p_mutation.nodeAliveToken.expired() == true`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 112, cols 8-36 - Cover the true outcome of `p_mutation.parent == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 117, cols 8-47 - Cover the true outcome in 2 instrumented instances of `p_mutation.hasParentAliveToken == false`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 127, cols 12-54 - Cover the false outcome of `_isMutationTargetAlive(p_mutation) == true`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 127, cols 58-100 - Cover the false outcome of `_isMutationParentAlive(p_mutation) == true`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 138, cols 12-48 - Cover the true outcome in 2 instrumented instances of `trait->_isTraversalBlocked() == true`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 143, cols 8-34 - Cover the true outcome in 4 instrumented instances and false outcome in 2 instrumented instances of `_traversalState == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 148, cols 8-58 - Cover the true outcome in 2 instrumented instances and false outcome in 3 instrumented instances of `p_mutation.type == DeferredMutationType::SetParent`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 154, cols 14-72 - Cover the true outcome in 3 instrumented instances and false outcome in 4 instrumented instances of `p_existingMutation.type == DeferredMutationType::SetParent`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 155, cols 11-53 - Cover the true outcome in 3 instrumented instances and false outcome in 4 instrumented instances of `p_existingMutation.node == p_mutation.node`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 158, cols 9-61 - Cover the true outcome in 3 instrumented instances and false outcome in 2 instrumented instances of `iterator != _traversalState->deferredMutations.end()`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 170, cols 8-34 - Cover the true outcome in 2 instrumented instances of `_traversalState == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 182, cols 10-45 - Cover the true outcome of `_isMutationValid(mutation) == false`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 187, cols 14-27 - Cover the false outcome in 2 instrumented instances of `mutation.type`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 189, cols 6-42 - Cover the false outcome of `case DeferredMutationType::SetParent`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 193, cols 6-46 - Cover the true outcome of `case DeferredMutationType::ClearChildren`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 205, cols 8-24 - Cover the true outcome in 3 instrumented instances of `p_parent == self`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 215, cols 31-67 - Cover the true outcome in 3 instrumented instances of `self->isAncestorOf(p_parent) == true`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 225, cols 9-48 - Cover the false outcome in 4 instrumented instances of `oldIterator != _parent->_children.end()`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 246, cols 11-37 - Cover the true outcome of `_children.empty() == false`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 250, cols 9-25 - Cover the true outcome in 4 instrumented instances and false outcome of `child == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 269, cols 9-27 - Cover the true outcome in 2 instrumented instances of `p_owner == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 269, cols 31-66 - Cover the true outcome in 2 instrumented instances of `p_owner->_traversalState == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 291, cols 9-25 - Cover the false outcome of `this != &p_other`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 310, cols 9-25 - Cover the true outcome of `state == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 315, cols 9-29 - Cover the false outcome in 2 instrumented instances of `state->nbBlocks != 0`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 321, cols 9-29 - Cover the false outcome of `state->nbBlocks == 0`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 321, cols 33-49 - Cover the false outcome in 2 instrumented instances of `owner != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 353, cols 8-34 - Cover the false outcome in 4 instrumented instances of `_traversalState != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 362, cols 8-34 - Cover the false outcome in 4 instrumented instances of `_traversalState != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 401, cols 9-52 - Cover the true outcome in 3 instrumented instances of `current == static_cast<const TType *>(this)`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 425, cols 8-31 - Cover the false outcome in 2 instrumented instances of `blockedOwner == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 430, cols 8-31 - Cover the true outcome in 2 instrumented instances of `blockedOwner != nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 441, cols 8-26 - Cover the true outcome of `p_child == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 451, cols 8-26 - Cover the true outcome of `p_child == nullptr`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 456, cols 8-54 - Cover the true outcome of `p_child->_parent != static_cast<TType *>(this)`.
- [ ] `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 467, cols 8-31 - Cover the true outcome and false outcome of `blockedOwner != nullptr`.

### includes/structures/game_engine/spk_animation_2d.hpp

Coverage: lines 0.00% (0/48, missing 48); branches - (0/0, missing 0); functions 0.00% (0/10, missing 10); regions 0.00% (0/10, missing 10).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 33-35 - Execute the currently uncalled function: `void addAnimation(const std::wstring &p_name, const Animation2D &p_animation)`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 38-40 - Execute the currently uncalled function: `[[nodiscard]] bool hasAnimation(const std::wstring &p_name) const`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 43-57 - Execute the currently uncalled function: `void play(const std::wstring &p_name)`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 60-62 - Execute the currently uncalled function: `void stop()`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 65-67 - Execute the currently uncalled function: `[[nodiscard]] bool isPlaying() const`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 70-72 - Execute the currently uncalled function: `[[nodiscard]] bool hasCurrent() const`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 75-77 - Execute the currently uncalled function: `[[nodiscard]] const std::wstring &currentName() const`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 80-88 - Execute the currently uncalled function: `[[nodiscard]] const Animation2D *currentAnimation() const`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 91-93 - Execute the currently uncalled function: `[[nodiscard]] double elapsedSeconds() const`.
- [ ] `includes/structures/game_engine/spk_animation_2d.hpp` - Lines 96-98 - Execute the currently uncalled function: `void addElapsed(double p_seconds)`.

### includes/structures/game_engine/spk_animation_logic.hpp

Coverage: lines 0.00% (0/35, missing 35); branches - (0/0, missing 0); functions 0.00% (0/1, missing 1); regions 0.00% (0/1, missing 1).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_animation_logic.hpp` - Lines 18-52 - Execute the currently uncalled function: `void _parseComponentForUpdate(const spk::UpdateTick &p_tick, AnimationController2D &p_controller) override`.

### includes/structures/game_engine/spk_component_2d.hpp

Coverage: lines 0.00% (0/2, missing 2); branches - (0/0, missing 0); functions 0.00% (0/2, missing 2); regions 0.00% (0/2, missing 2).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_component_2d.hpp` - Line 12 - Execute the currently uncalled function: `Component2D() = default;`.
- [ ] `includes/structures/game_engine/spk_component_2d.hpp` - Line 16 - Execute the currently uncalled function: `~Component2D() override = default;`.

### includes/structures/game_engine/spk_component_container.hpp

Coverage: lines 87.23% (41/47, missing 6); branches 83.33% (10/12, missing 2); functions 100.00% (8/8, missing 0); regions 91.67% (22/24, missing 2).

Uncovered code regions:

- [ ] `includes/structures/game_engine/spk_component_container.hpp` - Lines 47-49 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/game_engine/spk_component_container.hpp` - Lines 63-65 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `includes/structures/game_engine/spk_component_container.hpp` - Line 46, cols 8-35 - Cover the true outcome of `iterator == _byEngine.end()`.
- [ ] `includes/structures/game_engine/spk_component_container.hpp` - Line 62, cols 8-22 - Cover the true outcome of `p_from == p_to`.

### includes/structures/game_engine/spk_component_logic.hpp

Coverage: lines 96.02% (217/226, missing 9); branches 75.00% (24/32, missing 8); functions 100.00% (82/82, missing 0); regions 97.35% (110/113, missing 3).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 77-98 - Execute the currently uncalled function: `void (spk::ComponentLogic<TComponent>::*p_execute)(TEvent &))`. All 70 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 125-150 - Execute the currently uncalled function: `void onRender( spk::RenderUnitBuilder &p_builder, spk::ComponentRegistry &p_registry) final`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 135-137 - Execute the currently uncalled function: `[](const spk::Component *p_component) {`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 153-158 - Execute the currently uncalled function: `void onEvent(spk::WindowCloseRequestedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 161-166 - Execute the currently uncalled function: `void onEvent(spk::WindowDestroyedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 169-174 - Execute the currently uncalled function: `void onEvent(spk::WindowMovedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 177-182 - Execute the currently uncalled function: `void onEvent(spk::WindowResizedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 185-190 - Execute the currently uncalled function: `void onEvent(spk::WindowFocusGainedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 193-198 - Execute the currently uncalled function: `void onEvent(spk::WindowFocusLostEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 201-206 - Execute the currently uncalled function: `void onEvent(spk::WindowShownEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 209-214 - Execute the currently uncalled function: `void onEvent(spk::WindowHiddenEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 217-222 - Execute the currently uncalled function: `void onEvent(spk::MouseEnteredWindowEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 225-230 - Execute the currently uncalled function: `void onEvent(spk::MouseLeftWindowEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 233-238 - Execute the currently uncalled function: `void onEvent(spk::MouseMovedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 241-246 - Execute the currently uncalled function: `void onEvent(spk::MouseWheelScrolledEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 249-254 - Execute the currently uncalled function: `void onEvent(spk::MouseButtonPressedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 257-262 - Execute the currently uncalled function: `void onEvent(spk::MouseButtonReleasedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 265-270 - Execute the currently uncalled function: `void onEvent(spk::MouseButtonDoubleClickedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 273-278 - Execute the currently uncalled function: `void onEvent(spk::KeyPressedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 281-286 - Execute the currently uncalled function: `void onEvent(spk::KeyReleasedEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 289-294 - Execute the currently uncalled function: `void onEvent(spk::TextInputEvent &p_event, spk::ComponentRegistry &p_registry) final`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 297 - Execute the currently uncalled function: `virtual void _onUpdateStarted(const spk::UpdateTick &p_tick) { (void)p_tick; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 298 - Execute the currently uncalled function: `virtual void _parseComponentForUpdate(const spk::UpdateTick &p_tick, TComponent &p_component) { (void)p_tick; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 299 - Execute the currently uncalled function: `virtual void _executeUpdate(const spk::UpdateTick &p_tick) { (void)p_tick; }`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 301 - Execute the currently uncalled function: `virtual void _onRenderStarted(std::size_t p_componentCount) { (void)p_componentCount; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 302 - Execute the currently uncalled function: `virtual void _parseComponentForRender(TComponent &p_component) { (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 303 - Execute the currently uncalled function: `virtual void _executeRender(spk::RenderUnitBuilder &p_builder) { (void)p_builder; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 305 - Execute the currently uncalled function: `virtual void _onWindowCloseRequestedEventStarted(spk::WindowCloseRequestedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 306 - Execute the currently uncalled function: `virtual void _parseComponentForWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 307 - Execute the currently uncalled function: `virtual void _executeWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 309 - Execute the currently uncalled function: `virtual void _onWindowDestroyedEventStarted(spk::WindowDestroyedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 310 - Execute the currently uncalled function: `virtual void _parseComponentForWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 311 - Execute the currently uncalled function: `virtual void _executeWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 313 - Execute the currently uncalled function: `virtual void _onWindowMovedEventStarted(spk::WindowMovedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 314 - Execute the currently uncalled function: `virtual void _parseComponentForWindowMovedEvent(spk::WindowMovedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 315 - Execute the currently uncalled function: `virtual void _executeWindowMovedEvent(spk::WindowMovedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 317 - Execute the currently uncalled function: `virtual void _onWindowResizedEventStarted(spk::WindowResizedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 318 - Execute the currently uncalled function: `virtual void _parseComponentForWindowResizedEvent(spk::WindowResizedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 319 - Execute the currently uncalled function: `virtual void _executeWindowResizedEvent(spk::WindowResizedEvent &p_event) { (void)p_event; }`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 321 - Execute the currently uncalled function: `virtual void _onWindowFocusGainedEventStarted(spk::WindowFocusGainedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 322 - Execute the currently uncalled function: `virtual void _parseComponentForWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 323 - Execute the currently uncalled function: `virtual void _executeWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 325 - Execute the currently uncalled function: `virtual void _onWindowFocusLostEventStarted(spk::WindowFocusLostEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 326 - Execute the currently uncalled function: `virtual void _parseComponentForWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 327 - Execute the currently uncalled function: `virtual void _executeWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 329 - Execute the currently uncalled function: `virtual void _onWindowShownEventStarted(spk::WindowShownEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 330 - Execute the currently uncalled function: `virtual void _parseComponentForWindowShownEvent(spk::WindowShownEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 331 - Execute the currently uncalled function: `virtual void _executeWindowShownEvent(spk::WindowShownEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 333 - Execute the currently uncalled function: `virtual void _onWindowHiddenEventStarted(spk::WindowHiddenEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 334 - Execute the currently uncalled function: `virtual void _parseComponentForWindowHiddenEvent(spk::WindowHiddenEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 335 - Execute the currently uncalled function: `virtual void _executeWindowHiddenEvent(spk::WindowHiddenEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 337 - Execute the currently uncalled function: `virtual void _onMouseEnteredEventStarted(spk::MouseEnteredWindowEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 338 - Execute the currently uncalled function: `virtual void _parseComponentForMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 339 - Execute the currently uncalled function: `virtual void _executeMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 341 - Execute the currently uncalled function: `virtual void _onMouseLeftEventStarted(spk::MouseLeftWindowEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 342 - Execute the currently uncalled function: `virtual void _parseComponentForMouseLeftEvent(spk::MouseLeftWindowEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 343 - Execute the currently uncalled function: `virtual void _executeMouseLeftEvent(spk::MouseLeftWindowEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 345 - Execute the currently uncalled function: `virtual void _onMouseMovedEventStarted(spk::MouseMovedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 346 - Execute the currently uncalled function: `virtual void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 347 - Execute the currently uncalled function: `virtual void _executeMouseMovedEvent(spk::MouseMovedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 349 - Execute the currently uncalled function: `virtual void _onMouseWheelScrolledEventStarted(spk::MouseWheelScrolledEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 350 - Execute the currently uncalled function: `virtual void _parseComponentForMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 351 - Execute the currently uncalled function: `virtual void _executeMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 353 - Execute the currently uncalled function: `virtual void _onMouseButtonPressedEventStarted(spk::MouseButtonPressedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 354 - Execute the currently uncalled function: `virtual void _parseComponentForMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 355 - Execute the currently uncalled function: `virtual void _executeMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 357 - Execute the currently uncalled function: `virtual void _onMouseButtonReleasedEventStarted(spk::MouseButtonReleasedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 358 - Execute the currently uncalled function: `virtual void _parseComponentForMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 359 - Execute the currently uncalled function: `virtual void _executeMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 361 - Execute the currently uncalled function: `virtual void _onMouseButtonDoubleClickedEventStarted(spk::MouseButtonDoubleClickedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 362 - Execute the currently uncalled function: `virtual void _parseComponentForMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 363 - Execute the currently uncalled function: `virtual void _executeMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 365 - Execute the currently uncalled function: `virtual void _onKeyPressedEventStarted(spk::KeyPressedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 366 - Execute the currently uncalled function: `virtual void _parseComponentForKeyPressedEvent(spk::KeyPressedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 367 - Execute the currently uncalled function: `virtual void _executeKeyPressedEvent(spk::KeyPressedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 369 - Execute the currently uncalled function: `virtual void _onKeyReleasedEventStarted(spk::KeyReleasedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 370 - Execute the currently uncalled function: `virtual void _parseComponentForKeyReleasedEvent(spk::KeyReleasedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 371 - Execute the currently uncalled function: `virtual void _executeKeyReleasedEvent(spk::KeyReleasedEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 373 - Execute the currently uncalled function: `virtual void _onTextInputEventStarted(spk::TextInputEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 374 - Execute the currently uncalled function: `virtual void _parseComponentForTextInputEvent(spk::TextInputEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }`. All 4 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 375 - Execute the currently uncalled function: `virtual void _executeTextInputEvent(spk::TextInputEvent &p_event) { (void)p_event; }`. All 4 instrumented functions/instantiations at this location are unexecuted.

Uncovered code regions:

- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 79-81 - Exercise this uncovered code path: `return;`. This region is uncovered in 20 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 88-95 - Exercise this uncovered code path: `if (component == nullptr || component->isProcessable() == false || p_event.isConsumed() == true) continue; (this->*p_parse)(p_event, *static_cast<TComponent *>(component));`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89 - Exercise this uncovered code path: `component == nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89 - Exercise this uncovered code path: `component == nullptr || component->isProcessable() == false`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89 - Exercise this uncovered code path: `component == nullptr || component->isProcessable() == false || p_event.isConsumed() == true`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89 - Exercise this uncovered code path: `component->isProcessable() == false`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89 - Exercise this uncovered code path: `p_event.isConsumed() == true`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 90-92 - Exercise this uncovered code path: `continue;`. This region is uncovered in 18 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 94-95 - Exercise this uncovered code path: `(this->*p_parse)(p_event, *static_cast<TComponent *>(component));`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 103-105 - Exercise this uncovered code path: `return;`. This region is uncovered in 5 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 112-117 - Exercise this uncovered code path: `if (component != nullptr && component->isProcessable() == true) _parseComponentForUpdate(p_tick, *static_cast<TComponent *>(component));`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 113 - Exercise this uncovered code path: `component != nullptr`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 113 - Exercise this uncovered code path: `component != nullptr && component->isProcessable() == true`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 113 - Exercise this uncovered code path: `component->isProcessable() == true`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 114-116 - Exercise this uncovered code path: `_parseComponentForUpdate(p_tick, *static_cast<TComponent *>(component));`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Lines 127-129 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 78, cols 8-30 - Cover the true outcome in 90 instrumented instances and false outcome in 70 instrumented instances of `isActivated() == false`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 78, cols 34-62 - Cover the true outcome in 90 instrumented instances and false outcome in 70 instrumented instances of `p_event.isConsumed() == true`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 87, cols 35-36 - Cover the true outcome in 71 instrumented instances and false outcome in 70 instrumented instances of `:`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89, cols 9-29 - Cover the true outcome in 90 instrumented instances and false outcome in 71 instrumented instances of `component == nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89, cols 33-68 - Cover the true outcome in 89 instrumented instances and false outcome in 71 instrumented instances of `component->isProcessable() == false`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 89, cols 72-100 - Cover the true outcome in 88 instrumented instances and false outcome in 71 instrumented instances of `p_event.isConsumed() == true`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 102, cols 8-30 - Cover the true outcome in 5 instrumented instances of `isActivated() == false`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 111, cols 35-36 - Cover the true outcome in 3 instrumented instances of `:`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 113, cols 9-29 - Cover the true outcome in 3 instrumented instances and false outcome in 5 instrumented instances of `component != nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 113, cols 33-67 - Cover the true outcome in 3 instrumented instances and false outcome in 3 instrumented instances of `component->isProcessable() == true`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 126, cols 8-30 - Cover the true outcome in 5 instrumented instances and false outcome in 3 instrumented instances of `isActivated() == false`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 136, cols 13-35 - Cover the true outcome in 3 instrumented instances and false outcome in 5 instrumented instances of `p_component != nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 136, cols 39-75 - Cover the true outcome in 3 instrumented instances and false outcome in 4 instrumented instances of `p_component->isProcessable() == true`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 141, cols 35-36 - Cover the true outcome in 3 instrumented instances and false outcome in 3 instrumented instances of `:`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 143, cols 9-29 - Cover the true outcome in 3 instrumented instances and false outcome in 5 instrumented instances of `component != nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic.hpp` - Line 143, cols 33-67 - Cover the true outcome in 3 instrumented instances and false outcome in 4 instrumented instances of `component->isProcessable() == true`.

### includes/structures/game_engine/spk_component_logic_registry.hpp

Coverage: lines 100.00% (79/79, missing 0); branches 85.71% (24/28, missing 4); functions 100.00% (12/12, missing 0); regions 97.62% (41/42, missing 1).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 76 - Execute the currently uncalled function: `_priorityContracts.push_back(result.subscribeToPriorityChange([this]() { _orderDirty = true; }));`. All 4 instrumented functions/instantiations at this location are unexecuted.

Uncovered code regions:

- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Lines 69-71 - Exercise this uncovered code path: `return static_cast<TLogic &>(*iterator->second);`. This region is uncovered in 5 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 89 - Exercise this uncovered code path: `nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 89 - Exercise this uncovered code path: `static_cast<TLogic *>(iterator->second)`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 97 - Exercise this uncovered code path: `nullptr`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 97 - Exercise this uncovered code path: `static_cast<const TLogic *>(iterator->second)`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Lines 136-138 - Exercise this uncovered code path: `continue;`. This region is uncovered in 16 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 68, cols 8-33 - Cover the true outcome in 5 instrumented instances of `iterator != _lookup.end()`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 89, cols 12-37 - Cover the true outcome and false outcome in 2 instrumented instances of `iterator == _lookup.end()`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 97, cols 12-37 - Cover the true outcome in 2 instrumented instances and false outcome of `iterator == _lookup.end()`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 106, cols 9-25 - Cover the false outcome of `logic != nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 121, cols 9-25 - Cover the false outcome of `logic != nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 135, cols 9-25 - Cover the true outcome in 18 instrumented instances of `logic == nullptr`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 135, cols 29-58 - Cover the true outcome in 16 instrumented instances of `logic->isActivated() == false`.
- [ ] `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 135, cols 62-90 - Cover the true outcome in 17 instrumented instances of `p_event.isConsumed() == true`.

### includes/structures/game_engine/spk_component_registry.hpp

Coverage: lines 53.85% (7/13, missing 6); branches - (0/0, missing 0); functions 60.00% (3/5, missing 2); regions 66.67% (4/6, missing 2).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_component_registry.hpp` - Lines 25-27 - Execute the currently uncalled function: `[[nodiscard]] const spk::UUID &engineId() const`.
- [ ] `includes/structures/game_engine/spk_component_registry.hpp` - Lines 30-32 - Execute the currently uncalled function: `void setEngineId(const spk::UUID &p_engineId)`.

### includes/structures/game_engine/spk_entity.hpp

Coverage: lines 93.22% (55/59, missing 4); branches 87.50% (14/16, missing 2); functions 100.00% (6/6, missing 0); regions 100.00% (24/24, missing 0).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_entity.hpp` - Lines 62-76 - Execute the currently uncalled function: `TComponent &addComponent(TArguments &&...p_arguments)`.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Lines 97-107 - Execute the currently uncalled function: `[[nodiscard]] TComponent *component()`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Lines 112-122 - Execute the currently uncalled function: `[[nodiscard]] const TComponent *component() const`.

Uncovered code regions:

- [ ] `includes/structures/game_engine/spk_entity.hpp` - Lines 71-73 - Exercise this uncovered code path: `spk::ComponentStore::instance().add(std::type_index(typeid(*raw)), _engineId, raw);`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Lines 87-89 - Exercise this uncovered code path: `removeComponent(*found);`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Lines 101-103 - Exercise this uncovered code path: `return typed;`.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Lines 116-118 - Exercise this uncovered code path: `return typed;`.

Uncovered branch outcomes:

- [ ] `includes/structures/game_engine/spk_entity.hpp` - Line 70, cols 8-35 - Cover the true outcome in 7 instrumented instances and false outcome of `_engineId.isNull() == false`.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Line 86, cols 8-24 - Cover the true outcome in 2 instrumented instances and false outcome of `found != nullptr`.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Line 98, cols 54-55 - Cover the true outcome and false outcome of `:`.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Line 100, cols 70-86 - Cover the true outcome in 2 instrumented instances and false outcome in 2 instrumented instances of `typed != nullptr`.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Line 113, cols 54-55 - Cover the true outcome and false outcome of `:`.
- [ ] `includes/structures/game_engine/spk_entity.hpp` - Line 115, cols 82-98 - Cover the true outcome in 2 instrumented instances and false outcome in 2 instrumented instances of `typed != nullptr`.

### includes/structures/game_engine/spk_entity_2d.hpp

Coverage: lines 0.00% (0/19, missing 19); branches - (0/0, missing 0); functions 0.00% (0/5, missing 5); regions 0.00% (0/5, missing 5).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_entity_2d.hpp` - Lines 16-23 - Execute the currently uncalled function: `void _onParentChanged(spk::Entity *p_oldParent, spk::Entity *p_newParent) override`.
- [ ] `includes/structures/game_engine/spk_entity_2d.hpp` - Lines 29-30 - Execute the currently uncalled function: `_transform(&addComponent<Transform2D>())`.
- [ ] `includes/structures/game_engine/spk_entity_2d.hpp` - Lines 33-35 - Execute the currently uncalled function: `[[nodiscard]] Transform2D &transform()`.
- [ ] `includes/structures/game_engine/spk_entity_2d.hpp` - Lines 38-40 - Execute the currently uncalled function: `[[nodiscard]] const Transform2D &transform() const`.
- [ ] `includes/structures/game_engine/spk_entity_2d.hpp` - Lines 43-45 - Execute the currently uncalled function: `[[nodiscard]] const spk::Vector2 &position() const`.

### includes/structures/game_engine/spk_game_engine.hpp

Coverage: lines 100.00% (24/24, missing 0); branches 100.00% (4/4, missing 0); functions 100.00% (5/5, missing 0); regions 100.00% (11/11, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Uncovered code regions:

- [ ] `includes/structures/game_engine/spk_game_engine.hpp` - Lines 36-38 - Exercise this uncovered code path: `return;`. This region is uncovered in 16 instrumented functions/instantiations.
- [ ] `includes/structures/game_engine/spk_game_engine.hpp` - Lines 89-91 - Exercise this uncovered code path: `throw std::runtime_error("Requested logic does not exist");`.
- [ ] `includes/structures/game_engine/spk_game_engine.hpp` - Line 93 - Exercise this uncovered code path: `return *result`.

Uncovered branch outcomes:

- [ ] `includes/structures/game_engine/spk_game_engine.hpp` - Line 35, cols 8-30 - Cover the true outcome in 16 instrumented instances of `isActivated() == false`.
- [ ] `includes/structures/game_engine/spk_game_engine.hpp` - Line 88, cols 8-25 - Cover the true outcome and false outcome of `result == nullptr`.

### includes/structures/game_engine/spk_sprite_render_logic.hpp

Coverage: lines 0.00% (0/71, missing 71); branches - (0/0, missing 0); functions 0.00% (0/6, missing 6); regions 0.00% (0/6, missing 6).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_sprite_render_logic.hpp` - Lines 38-51 - Execute the currently uncalled function: `[[nodiscard]] InstancedSpriteRenderCommand &_commandFor( const spk::SpriteSheet &p_spriteSheet, const spk::TextureMesh2D &p_mesh)`.
- [ ] `includes/structures/game_engine/spk_sprite_render_logic.hpp` - Lines 55-57 - Execute the currently uncalled function: `[[nodiscard]] static std::size_t lastSpriteCount()`.
- [ ] `includes/structures/game_engine/spk_sprite_render_logic.hpp` - Lines 60-62 - Execute the currently uncalled function: `[[nodiscard]] static std::size_t lastPolygonCount()`.
- [ ] `includes/structures/game_engine/spk_sprite_render_logic.hpp` - Lines 66-74 - Execute the currently uncalled function: `void _onRenderStarted(std::size_t p_componentCount) override`.
- [ ] `includes/structures/game_engine/spk_sprite_render_logic.hpp` - Lines 77-96 - Execute the currently uncalled function: `void _parseComponentForRender(SpriteRenderer2D &p_sprite) override`.
- [ ] `includes/structures/game_engine/spk_sprite_render_logic.hpp` - Lines 99-120 - Execute the currently uncalled function: `void _executeRender(spk::RenderUnitBuilder &p_builder) override`.

### includes/structures/game_engine/spk_sprite_renderer_2d.hpp

Coverage: lines 0.00% (0/18, missing 18); branches - (0/0, missing 0); functions 0.00% (0/6, missing 6); regions 0.00% (0/6, missing 6).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_sprite_renderer_2d.hpp` - Lines 21-23 - Execute the currently uncalled function: `[[nodiscard]] const spk::SpriteSheet *spriteSheet() const`.
- [ ] `includes/structures/game_engine/spk_sprite_renderer_2d.hpp` - Lines 26-28 - Execute the currently uncalled function: `void setSpriteSheet(const spk::SpriteSheet *p_spriteSheet)`.
- [ ] `includes/structures/game_engine/spk_sprite_renderer_2d.hpp` - Lines 31-33 - Execute the currently uncalled function: `[[nodiscard]] const spk::TextureMesh2D *mesh() const`.
- [ ] `includes/structures/game_engine/spk_sprite_renderer_2d.hpp` - Lines 36-38 - Execute the currently uncalled function: `void setMesh(const spk::TextureMesh2D *p_mesh)`.
- [ ] `includes/structures/game_engine/spk_sprite_renderer_2d.hpp` - Lines 41-43 - Execute the currently uncalled function: `[[nodiscard]] const spk::SpriteSheet::Sprite &sprite() const`.
- [ ] `includes/structures/game_engine/spk_sprite_renderer_2d.hpp` - Lines 46-48 - Execute the currently uncalled function: `void setSprite(const spk::SpriteSheet::Sprite &p_sprite)`.

### includes/structures/game_engine/spk_transform_2d.hpp

Coverage: lines 0.00% (0/12, missing 12); branches - (0/0, missing 0); functions 0.00% (0/4, missing 4); regions 0.00% (0/4, missing 4).

Missing functions or function instantiations:

- [ ] `includes/structures/game_engine/spk_transform_2d.hpp` - Lines 41-43 - Execute the currently uncalled function: `[[nodiscard]] const spk::Vector2 &position() const`.
- [ ] `includes/structures/game_engine/spk_transform_2d.hpp` - Lines 46-48 - Execute the currently uncalled function: `[[nodiscard]] float rotation() const`.
- [ ] `includes/structures/game_engine/spk_transform_2d.hpp` - Lines 51-53 - Execute the currently uncalled function: `[[nodiscard]] const spk::Vector2 &scale() const`.
- [ ] `includes/structures/game_engine/spk_transform_2d.hpp` - Lines 56-58 - Execute the currently uncalled function: `[[nodiscard]] float layer() const`.

### includes/structures/graphics/geometry/spk_generic_mesh.hpp

Coverage: lines 71.08% (118/166, missing 48); branches 71.43% (20/28, missing 8); functions 78.79% (26/33, missing 7); regions 78.33% (47/60, missing 13).

Missing functions or function instantiations:

- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 39 - Execute the currently uncalled function: `_cache.configure([this]() { return _compute(); });`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 43-63 - Execute the currently uncalled function: `[[nodiscard]] std::uint32_t _compute() const`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 46-52 - Execute the currently uncalled function: `const auto mix = [&hash](std::span<const std::uint8_t> p_bytes) {`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 100-102 - Execute the currently uncalled function: `[[nodiscard]] bool isBound() const`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 105-109 - Execute the currently uncalled function: `void bind(const spk::BufferObject *p_vertexSource, const spk::BufferObject *p_indexSource) const`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 117-119 - Execute the currently uncalled function: `[[nodiscard]] std::uint32_t value() const`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 287-293 - Execute the currently uncalled function: `[[nodiscard]] std::uint32_t hashKey() const`.

Uncovered code regions:

- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 134-136 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 142-144 - Exercise this uncovered code path: `throw std::overflow_error("spk::GenericMesh: too many vertices for uint32_t indexes");`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 172-174 - Exercise this uncovered code path: `return it->second;`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 231-233 - Exercise this uncovered code path: `throw std::runtime_error("Can't add a shape with less than 3 vertices in a mesh");`.

Uncovered branch outcomes:

- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 47, cols 35-36 - Cover the true outcome in 3 instrumented instances and false outcome in 3 instrumented instances of `:`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 54, cols 9-33 - Cover the true outcome in 3 instrumented instances and false outcome in 3 instrumented instances of `_vertexSource != nullptr`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 58, cols 9-32 - Cover the true outcome in 3 instrumented instances and false outcome in 3 instrumented instances of `_indexSource != nullptr`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 133, cols 8-26 - Cover the true outcome in 2 instrumented instances of `p_vertexCount == 0`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 141, cols 8-37 - Cover the true outcome in 3 instrumented instances of `currentVertexCount > maxIndex`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 141, cols 41-90 - Cover the true outcome in 3 instrumented instances of `p_vertexCount - 1 > maxIndex - currentVertexCount`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 159, cols 9-41 - Cover the false outcome of `_vertexLookup.count(vertex) == 0`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 159, cols 9-74 - Cover the false outcome of `_vertexLookup.count(vertex) == 0 && seenNew.insert(vertex).second`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 159, cols 45-74 - Cover the false outcome in 2 instrumented instances of `seenNew.insert(vertex).second`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 171, cols 8-33 - Cover the true outcome of `it != _vertexLookup.end()`.
- [ ] `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 230, cols 8-29 - Cover the true outcome of `p_vertices.size() < 3`.

### includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp

Coverage: lines 78.71% (122/155, missing 33); branches 61.36% (27/44, missing 17); functions 83.33% (15/18, missing 3); regions 75.38% (49/65, missing 16).

Missing functions or function instantiations:

- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 74-77 - Execute the currently uncalled function: `[[nodiscard]] bool _isDeadForeignContext(const Entry &p_entry, std::uint64_t p_currentContextId) const`. All 6 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 80-87 - Execute the currently uncalled function: `void _removeEntryAt(std::size_t p_index)`. All 6 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 90-99 - Execute the currently uncalled function: `void _removeEntry(Entry *p_entry)`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 264-265 - Execute the currently uncalled function: `[](TGpuObject &) {`. All 6 instrumented functions/instantiations at this location are unexecuted.

Uncovered code regions:

- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 61 - Exercise this uncovered code path: `return nullptr`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 70 - Exercise this uncovered code path: `p_entry->object->contentVersion() == p_contentVersion`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 94-96 - Exercise this uncovered code path: `*p_entry = std::move(_entries.back());`. This region is uncovered in 4 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 110-123 - Exercise this uncovered code path: `for (std::size_t index = 0; index < _entries.size();) if (_isDeadForeignContext(_entries[index], p_currentContextId) == true) _removeEntryAt(index);`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 110 - Exercise this uncovered code path: `index < _entries.size()`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 111-120 - Exercise this uncovered code path: `if (_isDeadForeignContext(_entries[index], p_currentContextId) == true) _removeEntryAt(index); else`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 112 - Exercise this uncovered code path: `_isDeadForeignContext(_entries[index], p_currentContextId) == true`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 113-115 - Exercise this uncovered code path: `_removeEntryAt(index);`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 117-119 - Exercise this uncovered code path: `++index;`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 148 - Exercise this uncovered code path: `p_entry->object->version() != p_version`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 149-152 - Exercise this uncovered code path: `_removeEntry(p_entry); p_entry = nullptr;`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 163 - Exercise this uncovered code path: `return *p_entry`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 177-179 - Exercise this uncovered code path: `std::forward<TRefresh>(p_refresh)(*p_entry.object); p_entry.object->_contentVersion = p_contentVersion;`. This region is uncovered in 6 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 229-231 - Exercise this uncovered code path: `return *entry->object;`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 234-236 - Exercise this uncovered code path: `entry = _findEntry(contextId);`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 274-276 - Exercise this uncovered code path: `return nullptr;`. This region is uncovered in 2 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 42, cols 9-39 - Cover the false outcome in 6 instrumented instances of `entry.contextId == p_contextId`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 53, cols 28-29 - Cover the false outcome in 2 instrumented instances of `:`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 55, cols 9-39 - Cover the false outcome in 5 instrumented instances of `entry.contextId == p_contextId`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 69, cols 33-72 - Cover the true outcome and false outcome in 2 instrumented instances of `p_entry->object->version() == p_version`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 70, cols 8-61 - Cover the true outcome and false outcome in 5 instrumented instances of `p_entry->object->contentVersion() == p_contentVersion`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 75, cols 11-50 - Cover the true outcome in 6 instrumented instances and false outcome in 6 instrumented instances of `p_entry.contextId != p_currentContextId`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 76, cols 8-63 - Cover the true outcome in 6 instrumented instances and false outcome in 6 instrumented instances of `spk::OpenGL::isContextAlive(p_entry.contextId) == false`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 81, cols 8-38 - Cover the true outcome in 6 instrumented instances and false outcome in 6 instrumented instances of `p_index != _entries.size() - 1`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 93, cols 8-35 - Cover the true outcome in 6 instrumented instances and false outcome in 2 instrumented instances of `p_entry != &_entries.back()`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 105, cols 8-48 - Cover the false outcome in 6 instrumented instances of `currentGeneration == _prunedAtGeneration`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 110, cols 32-55 - Cover the true outcome in 6 instrumented instances and false outcome in 6 instrumented instances of `index < _entries.size()`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 112, cols 9-75 - Cover the true outcome in 6 instrumented instances and false outcome in 6 instrumented instances of `_isDeadForeignContext(_entries[index], p_currentContextId) == true`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 148, cols 8-26 - Cover the true outcome in 2 instrumented instances of `p_entry != nullptr`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 148, cols 30-69 - Cover the true outcome in 3 instrumented instances and false outcome in 6 instrumented instances of `p_entry->object->version() != p_version`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 154, cols 8-26 - Cover the false outcome in 6 instrumented instances of `p_entry == nullptr`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 172, cols 8-60 - Cover the false outcome in 6 instrumented instances of `p_entry.object->contentVersion() == p_contentVersion`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 228, cols 8-72 - Cover the true outcome of `_hasExpectedVersions(entry, p_version, p_contentVersion) == true`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 233, cols 8-52 - Cover the true outcome in 7 instrumented instances of `_pruneDeadForeignContexts(contextId) == true`.
- [ ] `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 273, cols 8-24 - Cover the true outcome in 2 instrumented instances of `entry == nullptr`.

### includes/structures/graphics/rendering/unit/spk_render_unit_builder.hpp

Coverage: lines 83.33% (15/18, missing 3); branches 50.00% (1/2, missing 1); functions 100.00% (4/4, missing 0); regions 100.00% (6/6, missing 0).

Missing functions or function instantiations:

- [ ] `includes/structures/graphics/rendering/unit/spk_render_unit_builder.hpp` - Lines 44-53 - Execute the currently uncalled function: `TCommand &emplace(TArguments &&...p_arguments)`.

Uncovered branch outcomes:

- [ ] `includes/structures/graphics/rendering/unit/spk_render_unit_builder.hpp` - Line 33, cols 8-28 - Cover the false outcome of `p_command != nullptr`.

### includes/structures/graphics/spk_layout_buffer_object.hpp

Coverage: lines 89.47% (17/19, missing 2); branches 75.00% (3/4, missing 1); functions 100.00% (4/4, missing 0); regions 90.00% (9/10, missing 1).

Uncovered code regions:

- [ ] `includes/structures/graphics/spk_layout_buffer_object.hpp` - Lines 88-91 - Exercise this uncovered code path: `setVertexBytes(nullptr, 0); return;`.
- [ ] `includes/structures/graphics/spk_layout_buffer_object.hpp` - Lines 92-93 - Exercise this uncovered code path: `setVertexBytes(p_vertices.data(), p_vertices.size_bytes());`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/spk_layout_buffer_object.hpp` - Lines 99-101 - Exercise this uncovered code path: `return;`. This region is uncovered in 3 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/graphics/spk_layout_buffer_object.hpp` - Line 87, cols 8-34 - Cover the true outcome and false outcome in 3 instrumented instances of `p_vertices.empty() == true`.
- [ ] `includes/structures/graphics/spk_layout_buffer_object.hpp` - Line 98, cols 8-34 - Cover the true outcome in 3 instrumented instances of `p_vertices.empty() == true`.

### includes/structures/graphics/spk_shader_storage_buffer_object.hpp

Coverage: lines 100.00% (62/62, missing 0); branches - (0/0, missing 0); functions 100.00% (6/6, missing 0); regions 100.00% (6/6, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Missing functions or function instantiations:

- [ ] `includes/structures/graphics/spk_shader_storage_buffer_object.hpp` - Lines 58-69 - Execute the currently uncalled function: `TType &emplaceBack(TArgs &&...p_args)`.

### includes/structures/graphics/spk_uniform.hpp

Coverage: lines 100.00% (144/144, missing 0); branches 92.86% (13/14, missing 1); functions 100.00% (49/49, missing 0); regions 99.29% (140/141, missing 1).

Uncovered code regions:

- [ ] `includes/structures/graphics/spk_uniform.hpp` - Line 92 - Exercise this uncovered code path: `0`.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Lines 375-377 - Exercise this uncovered code path: `throw std::runtime_error("ArrayUniform: count cannot be zero");`. This region is uncovered in 15 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Line 382 - Exercise this uncovered code path: `p_count > 0`. This region is uncovered in 15 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Lines 383-385 - Exercise this uncovered code path: `throw std::runtime_error("ArrayUniform: null value array");`. This region is uncovered in 15 instrumented functions/instantiations.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Lines 387-389 - Exercise this uncovered code path: `throw std::runtime_error("ArrayUniform: count mismatch");`. This region is uncovered in 15 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/graphics/spk_uniform.hpp` - Line 92, cols 28-35 - Cover the false outcome of `p_value`.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Line 374, cols 8-20 - Cover the true outcome in 15 instrumented instances of `p_count == 0`.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Line 382, cols 8-27 - Cover the true outcome in 15 instrumented instances of `p_values == nullptr`.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Line 382, cols 31-42 - Cover the true outcome in 15 instrumented instances and false outcome in 15 instrumented instances of `p_count > 0`.
- [ ] `includes/structures/graphics/spk_uniform.hpp` - Line 386, cols 8-33 - Cover the true outcome in 15 instrumented instances of `p_count != _values.size()`.

### includes/structures/math/spk_matrix.hpp

Coverage: lines 97.80% (311/318, missing 7); branches 98.75% (79/80, missing 1); functions 92.86% (26/28, missing 2); regions 97.90% (140/143, missing 3).

Missing functions or function instantiations:

- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 95-97 - Execute the currently uncalled function: `[[nodiscard]] static IMatrix identity()`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 244-246 - Execute the currently uncalled function: `[[nodiscard]] static IMatrix translation(const spk::Vector3 &p_translation)`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 258-260 - Execute the currently uncalled function: `[[nodiscard]] static IMatrix scale(const spk::Vector3 &p_scale)`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 392-442 - Execute the currently uncalled function: `[[nodiscard]] IMatrix inverse() const`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 445-464 - Execute the currently uncalled function: `friend std::ostream &operator<<(std::ostream &p_outputStream, const IMatrix &p_matrix)`.

Uncovered code regions:

- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 35-37 - Exercise this uncovered code path: `throw std::invalid_argument("spk::IMatrix: row index out of bounds");`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 44-46 - Exercise this uncovered code path: `throw std::invalid_argument("spk::IMatrix: row index out of bounds");`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 80-82 - Exercise this uncovered code path: `throw std::invalid_argument("spk::IMatrix: initializer list size does not match matrix dimensions");`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 102-104 - Exercise this uncovered code path: `throw std::invalid_argument("spk::IMatrix: column index out of bounds");`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 111-113 - Exercise this uncovered code path: `throw std::invalid_argument("spk::IMatrix: column index out of bounds");`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/math/spk_matrix.hpp` - Lines 179-181 - Exercise this uncovered code path: `return false;`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 184 - Exercise this uncovered code path: `return true`.

Uncovered branch outcomes:

- [ ] `includes/structures/math/spk_matrix.hpp` - Line 34, cols 9-25 - Cover the true outcome in 2 instrumented instances of `p_index >= SizeY`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 43, cols 9-25 - Cover the true outcome in 2 instrumented instances of `p_index >= SizeY`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 79, cols 8-40 - Cover the true outcome in 2 instrumented instances of `p_values.size() != SizeX * SizeY`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 101, cols 8-24 - Cover the true outcome in 2 instrumented instances of `p_index >= SizeX`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 110, cols 8-24 - Cover the true outcome in 2 instrumented instances of `p_index >= SizeX`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 174, cols 28-37 - Cover the false outcome of `x < SizeX`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 178, cols 10-58 - Cover the true outcome of `spk::ApproxValue((*this)[x][y]) != p_other[x][y]`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 396, cols 28-33 - Cover the true outcome and false outcome of `i < N`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 400, cols 35-42 - Cover the true outcome and false outcome of `row < N`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 403, cols 10-28 - Cover the true outcome and false outcome of `value > maxElement`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 409, cols 9-22 - Cover the true outcome and false outcome of `pivotRow != i`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 411, cols 35-45 - Cover the true outcome and false outcome of `column < N`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 418, cols 9-38 - Cover the true outcome and false outcome of `std::fabs(pivotValue) < 1e-9f`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 422, cols 34-44 - Cover the true outcome and false outcome of `column < N`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 427, cols 31-38 - Cover the true outcome and false outcome of `row < N`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 429, cols 10-18 - Cover the true outcome and false outcome of `row == i`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 434, cols 35-45 - Cover the true outcome and false outcome of `column < N`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 446, cols 28-37 - Cover the true outcome and false outcome of `y < SizeY`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 448, cols 9-15 - Cover the true outcome and false outcome of `y != 0`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 453, cols 29-38 - Cover the true outcome and false outcome of `x < SizeX`.
- [ ] `includes/structures/math/spk_matrix.hpp` - Line 455, cols 10-16 - Cover the true outcome and false outcome of `x != 0`.

### includes/structures/math/spk_poisson_disk.hpp

Coverage: lines 100.00% (29/29, missing 0); branches 100.00% (8/8, missing 0); functions 100.00% (2/2, missing 0); regions 100.00% (12/12, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Uncovered code regions:

- [ ] `includes/structures/math/spk_poisson_disk.hpp` - Lines 60-62 - Exercise this uncovered code path: `return false;`.
- [ ] `includes/structures/math/spk_poisson_disk.hpp` - Lines 68-70 - Exercise this uncovered code path: `return false;`.

Uncovered branch outcomes:

- [ ] `includes/structures/math/spk_poisson_disk.hpp` - Line 59, cols 9-25 - Cover the true outcome of `p_point.x < 0.0f`.
- [ ] `includes/structures/math/spk_poisson_disk.hpp` - Line 59, cols 29-45 - Cover the true outcome of `p_point.y < 0.0f`.
- [ ] `includes/structures/math/spk_poisson_disk.hpp` - Line 67, cols 9-28 - Cover the true outcome of `x >= p_grid.width()`.
- [ ] `includes/structures/math/spk_poisson_disk.hpp` - Line 67, cols 32-52 - Cover the true outcome of `y >= p_grid.height()`.

### includes/structures/math/spk_vector2.hpp

Coverage: lines 100.00% (253/253, missing 0); branches 100.00% (48/48, missing 0); functions 100.00% (51/51, missing 0); regions 100.00% (103/103, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Missing functions or function instantiations:

- [ ] `includes/structures/math/spk_vector2.hpp` - Lines 69-72 - Execute the currently uncalled function: `friend std::ostream &operator<<(std::ostream &p_outputStream, const IVector2<TType> &p_value)`.

Uncovered code regions:

- [ ] `includes/structures/math/spk_vector2.hpp` - Lines 129-131 - Exercise this uncovered code path: `throw std::invalid_argument("spk::IVector2: division by zero scalar");`.

Uncovered branch outcomes:

- [ ] `includes/structures/math/spk_vector2.hpp` - Line 128, cols 8-39 - Cover the true outcome of `spk::ApproxValue(p_scalar) == 0`.
- [ ] `includes/structures/math/spk_vector2.hpp` - Line 413, cols 31-50 - Cover the true outcome of `p_value.y <= high.y`.

### includes/structures/math/spk_vector3.hpp

Coverage: lines 100.00% (230/230, missing 0); branches 100.00% (54/54, missing 0); functions 100.00% (43/43, missing 0); regions 100.00% (108/108, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Missing functions or function instantiations:

- [ ] `includes/structures/math/spk_vector3.hpp` - Lines 86-89 - Execute the currently uncalled function: `friend std::ostream &operator<<(std::ostream &p_outputStream, const IVector3<TType> &p_value)`.

### includes/structures/math/spk_vector4.hpp

Coverage: lines 100.00% (235/235, missing 0); branches 100.00% (60/60, missing 0); functions 100.00% (45/45, missing 0); regions 100.00% (121/121, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Missing functions or function instantiations:

- [ ] `includes/structures/math/spk_vector4.hpp` - Lines 109-112 - Execute the currently uncalled function: `friend std::ostream &operator<<(std::ostream &p_outputStream, const IVector4<TType> &p_value)`.

### includes/structures/system/event/spk_events.hpp

Coverage: lines 100.00% (38/38, missing 0); branches - (0/0, missing 0); functions 100.00% (13/13, missing 0); regions 100.00% (17/17, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Missing functions or function instantiations:

- [ ] `includes/structures/system/event/spk_events.hpp` - Lines 51-53 - Execute the currently uncalled function: `void consume()`. All 9 instrumented functions/instantiations at this location are unexecuted.

### includes/structures/system/thread/spk_thread_safe_contract.hpp

Coverage: lines 94.74% (90/95, missing 5); branches 87.50% (14/16, missing 2); functions 100.00% (15/15, missing 0); regions 97.67% (42/43, missing 1).

Uncovered code regions:

- [ ] `includes/structures/system/thread/spk_thread_safe_contract.hpp` - Lines 53-58 - Exercise this uncovered code path: `release(); _blocker = std::move(p_other._blocker); _mutex = std::move(p_other._mutex);`.
- [ ] `includes/structures/system/thread/spk_thread_safe_contract.hpp` - Lines 115-120 - Exercise this uncovered code path: `resign(); _contract = std::move(p_other._contract); _mutex = std::move(p_other._mutex);`.
- [ ] `includes/structures/system/thread/spk_thread_safe_contract.hpp` - Lines 173-175 - Exercise this uncovered code path: `return false;`.

Uncovered branch outcomes:

- [ ] `includes/structures/system/thread/spk_thread_safe_contract.hpp` - Line 52, cols 9-25 - Cover the true outcome of `this != &p_other`.
- [ ] `includes/structures/system/thread/spk_thread_safe_contract.hpp` - Line 114, cols 8-24 - Cover the true outcome and false outcome of `this != &p_other`.
- [ ] `includes/structures/system/thread/spk_thread_safe_contract.hpp` - Line 172, cols 8-24 - Cover the true outcome of `mutex == nullptr`.

### includes/structures/system/thread/spk_thread_safe_deque.hpp

Coverage: lines 100.00% (45/45, missing 0); branches 100.00% (4/4, missing 0); functions 100.00% (8/8, missing 0); regions 100.00% (12/12, missing 0).

Aggregate metrics are 100%, but the HTML detail contains the partially uncovered expansions listed below.

Uncovered code regions:

- [ ] `includes/structures/system/thread/spk_thread_safe_deque.hpp` - Lines 45-47 - Exercise this uncovered code path: `return false;`. This region is uncovered in 2 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/system/thread/spk_thread_safe_deque.hpp` - Line 44, cols 8-31 - Cover the true outcome in 2 instrumented instances of `_values.empty() == true`.

### includes/structures/widget/spk_grid_layout.hpp

Coverage: lines 83.89% (380/453, missing 73); branches 78.41% (138/176, missing 38); functions 92.86% (26/28, missing 2); regions 88.75% (213/240, missing 27).

Missing functions or function instantiations:

- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 250-252 - Execute the currently uncalled function: `configureFixedSizeGenerator([this]() {`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 256-258 - Execute the currently uncalled function: `[[nodiscard]] spk::Vector2UInt preferredSizeFor(const spk::Vector2UInt &p_availableSize) const override`.

Uncovered code regions:

- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 29-31 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 138-140 - Exercise this uncovered code path: `return {0u, 0u};`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 150 - Exercise this uncovered code path: `0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 158 - Exercise this uncovered code path: `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 169-173 - Exercise this uncovered code path: `case SizePolicy::Fixed: const spk::Vector2UInt fixedSizeValue = element->fixedSize(); columnWidth[x] = std::max(columnWidth[x], static_cast<size_t>(fixedSizeValue.x));`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 175-178 - Exercise this uncovered code path: `case SizePolicy::Maximum: columnWidth[x] = std::max(columnWidth[x], static_cast<size_t>(maximalSize.x)); break;`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 199 - Exercise this uncovered code path: `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 208-212 - Exercise this uncovered code path: `case SizePolicy::Fixed: const spk::Vector2UInt fixedSizeValue = element->fixedSize(); rowHeight[y] = std::max(rowHeight[y], static_cast<size_t>(fixedSizeValue.y));`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 214-218 - Exercise this uncovered code path: `case SizePolicy::Maximum: const spk::Vector2UInt maximalSizeValue = element->maximalSize(); rowHeight[y] = std::max(rowHeight[y], static_cast<size_t>(maximalSizeValue.y));`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 278 - Exercise this uncovered code path: `_size.x`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 283 - Exercise this uncovered code path: `_size.y`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 301-303 - Exercise this uncovered code path: `*_elements[index] = Element(p_widget, p_sizePolicy, spk::Vector2UInt{1, 1});`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 333 - Exercise this uncovered code path: `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 334-336 - Exercise this uncovered code path: `continue;`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 416 - Exercise this uncovered code path: `0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 435 - Exercise this uncovered code path: `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 436-438 - Exercise this uncovered code path: `continue;`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 476 - Exercise this uncovered code path: `0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 516-518 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 535-536 - Exercise this uncovered code path: `_ensureSize(p_row + 1, NbColumns); return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 547-549 - Exercise this uncovered code path: `return;`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 566-567 - Exercise this uncovered code path: `_ensureSize(NbRows, p_column + 1); return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 577 - Exercise this uncovered code path: `NbRows == 0`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Lines 578-580 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 603 - Exercise this uncovered code path: `return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy)`.

Uncovered branch outcomes:

- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 28, cols 29-49 - Cover the true outcome of `p_columns == _size.x`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 137, cols 8-20 - Cover the true outcome of `_size.y == 0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 137, cols 24-36 - Cover the true outcome of `_size.x == 0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 150, cols 5-40 - Cover the false outcome of `(p_availableSize.y > totalPaddingY)`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 158, cols 33-61 - Cover the true outcome of `element->widget() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 158, cols 65-93 - Cover the true outcome and false outcome of `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 169, cols 6-28 - Cover the true outcome of `case SizePolicy::Fixed`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 175, cols 6-30 - Cover the true outcome of `case SizePolicy::Maximum`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 180, cols 6-13 - Cover the false outcome of `default`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 199, cols 33-61 - Cover the true outcome of `element->widget() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 199, cols 65-93 - Cover the true outcome and false outcome of `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 208, cols 6-28 - Cover the true outcome of `case SizePolicy::Fixed`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 214, cols 6-30 - Cover the true outcome of `case SizePolicy::Maximum`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 220, cols 6-13 - Cover the false outcome of `default`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 278, cols 29-43 - Cover the false outcome of `(_size.x == 0)`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 283, cols 16-30 - Cover the false outcome of `(_size.y == 0)`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 296, cols 8-35 - Cover the false outcome of `_elements[index] == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 309, cols 24-36 - Cover the true outcome of `_size.y == 0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 333, cols 10-28 - Cover the true outcome of `element == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 333, cols 33-61 - Cover the true outcome of `element->widget() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 333, cols 65-93 - Cover the true outcome and false outcome of `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 356, cols 11-38 - Cover the false outcome of `isExtendableOnX[x] == false`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 365, cols 11-38 - Cover the false outcome of `isExtendableOnY[y] == false`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 416, cols 17-43 - Cover the false outcome of `(spaceLeftX >= sumColsMin)`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 435, cols 10-28 - Cover the true outcome of `element == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 435, cols 33-61 - Cover the true outcome of `element->widget() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 435, cols 65-93 - Cover the true outcome and false outcome of `element->layout() == nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 476, cols 17-43 - Cover the false outcome of `(spaceLeftY >= sumRowsMin)`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 495, cols 10-28 - Cover the false outcome of `element != nullptr`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 515, cols 8-22 - Cover the true outcome of `NbColumns == 0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 531, cols 8-29 - Cover the false outcome of `p_column >= NbColumns`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 546, cols 8-19 - Cover the true outcome of `NbRows == 0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 562, cols 8-23 - Cover the false outcome of `p_row >= NbRows`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 577, cols 8-22 - Cover the true outcome in 2 instrumented instances and false outcome in 2 instrumented instances of `NbColumns == 0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 577, cols 26-37 - Cover the true outcome in 2 instrumented instances and false outcome in 2 instrumented instances of `NbRows == 0`.
- [ ] `includes/structures/widget/spk_grid_layout.hpp` - Line 599, cols 33-48 - Cover the false outcome of `p_row >= NbRows`.

### includes/structures/widget/spk_linear_layout.hpp

Coverage: lines 94.19% (243/258, missing 15); branches 84.72% (61/72, missing 11); functions 95.65% (22/23, missing 1); regions 92.71% (89/96, missing 7).

Missing functions or function instantiations:

- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 375-377 - Execute the currently uncalled function: `configureFixedSizeGenerator([this]() {`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 381-383 - Execute the currently uncalled function: `[[nodiscard]] spk::Vector2UInt preferredSizeFor(const spk::Vector2UInt &p_availableSize) const override`.

Uncovered code regions:

- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 58 - Exercise this uncovered code path: `p_element->layout() != nullptr`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 122-124 - Exercise this uncovered code path: `case SizePolicy::Fixed: p_item.size = p_requestedSize; break;`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 130-132 - Exercise this uncovered code path: `case SizePolicy::Maximum: p_item.size = p_item.maxP; break;`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 182 - Exercise this uncovered code path: `0`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 191-193 - Exercise this uncovered code path: `continue;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 229-235 - Exercise this uncovered code path: `for (auto &it : p_items) it.extend = true; extendCount = p_items.size();`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 231-233 - Exercise this uncovered code path: `it.extend = true;`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 247 - Exercise this uncovered code path: `1`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 249-251 - Exercise this uncovered code path: `--remain;`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 324-326 - Exercise this uncovered code path: `return {0u, 0u};`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 330-332 - Exercise this uncovered code path: `return {0u, 0u};`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Lines 394-396 - Exercise this uncovered code path: `return;`. This region is uncovered in 2 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 58, cols 11-31 - Cover the false outcome in 2 instrumented instances of `p_element != nullptr`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 58, cols 36-66 - Cover the false outcome in 2 instrumented instances of `p_element->widget() != nullptr`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 58, cols 70-100 - Cover the true outcome in 2 instrumented instances and false outcome in 2 instrumented instances of `p_element->layout() != nullptr`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 120, cols 12-24 - Cover the false outcome in 2 instrumented instances of `p_sizePolicy`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 122, cols 4-26 - Cover the true outcome of `case SizePolicy::Fixed`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 130, cols 4-28 - Cover the true outcome of `case SizePolicy::Maximum`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 182, cols 47-73 - Cover the false outcome in 2 instrumented instances of `_elements.empty() == false`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 190, cols 9-42 - Cover the true outcome in 2 instrumented instances of `_isValidElement(element) == false`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 214, cols 24-47 - Cover the true outcome in 2 instrumented instances of `p_items.empty() == true`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 228, cols 8-24 - Cover the true outcome of `extendCount == 0`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 230, cols 19-20 - Cover the true outcome and false outcome of `:`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 247, cols 24-34 - Cover the true outcome of `remain > 0`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 248, cols 9-19 - Cover the true outcome of `remain > 0`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 323, cols 8-33 - Cover the true outcome in 2 instrumented instances of `_elements.empty() == true`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 329, cols 8-29 - Cover the true outcome in 2 instrumented instances of `items.empty() == true`.
- [ ] `includes/structures/widget/spk_linear_layout.hpp` - Line 393, cols 8-29 - Cover the true outcome in 2 instrumented instances of `items.empty() == true`.

### includes/structures/widget/spk_numeric_spin_box.hpp

Coverage: lines 87.12% (142/163, missing 21); branches 50.00% (17/34, missing 17); functions 95.00% (19/20, missing 1); regions 79.41% (54/68, missing 14).

Missing functions or function instantiations:

- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 44-59 - Execute the currently uncalled function: `[[nodiscard]] static bool _isEmptyOrSignOnly(const std::string &p_text)`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 62-112 - Execute the currently uncalled function: `[[nodiscard]] static ValidationState _parseValue(const std::string &p_text, value_type &p_outValue)`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 164-178 - Execute the currently uncalled function: `[](const spk::Font::Text &p_text) {`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 184-186 - Execute the currently uncalled function: `_raiseContract = _raiseButton.subscribeToClick([this]() {`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 188-190 - Execute the currently uncalled function: `_lowerContract = _lowerButton.subscribeToClick([this]() {`. All 2 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 194-202 - Execute the currently uncalled function: `configureMinimalSizeGenerator([this]() {`. All 3 instrumented functions/instantiations at this location are unexecuted.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 213-216 - Execute the currently uncalled function: `void setValue(const value_type &p_value)`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 234-236 - Execute the currently uncalled function: `void increase()`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 239-241 - Execute the currently uncalled function: `void decrease()`. All 2 instrumented functions/instantiations at this location are unexecuted.

Uncovered code regions:

- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45 - Exercise this uncovered code path: `p_text == "-"`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45 - Exercise this uncovered code path: `p_text == "+"`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 46-48 - Exercise this uncovered code path: `return true;`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 50-58 - Exercise this uncovered code path: `if constexpr (std::is_floating_point_v<value_type>) if (p_text == "." || p_text == "-." || p_text == "+.") return true;`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 72-74 - Exercise this uncovered code path: `return ValidationState::Undefined;`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 80 - Exercise this uncovered code path: `begin + 1`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 84 - Exercise this uncovered code path: `ptr != end`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 85-87 - Exercise this uncovered code path: `return ValidationState::Invalid;`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 133-138 - Exercise this uncovered code path: `value_type parsedValue{}; if (_parseValue(_valueEdit.textAsUTF8(), parsedValue) == ValidationState::Valid) _value = parsedValue;`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 134 - Exercise this uncovered code path: `_parseValue(_valueEdit.textAsUTF8(), parsedValue) == ValidationState::Valid`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 135-137 - Exercise this uncovered code path: `_value = parsedValue;`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 145 - Exercise this uncovered code path: `0`. This region is uncovered in 3 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 170-172 - Exercise this uncovered code path: `return ValidationState::Invalid;`. This region is uncovered in 2 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45, cols 8-30 - Cover the true outcome in 2 instrumented instances and false outcome in 2 instrumented instances of `p_text.empty() == true`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45, cols 34-47 - Cover the true outcome in 3 instrumented instances and false outcome in 2 instrumented instances of `p_text == "-"`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45, cols 51-64 - Cover the true outcome in 3 instrumented instances and false outcome in 2 instrumented instances of `p_text == "+"`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 52, cols 9-22 - Cover the true outcome and false outcome of `p_text == "."`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 52, cols 26-40 - Cover the true outcome and false outcome of `p_text == "-."`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 52, cols 44-58 - Cover the true outcome and false outcome of `p_text == "+."`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 65, cols 36-57 - Cover the false outcome of `p_text.front() == '-'`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 71, cols 8-42 - Cover the true outcome in 2 instrumented instances and false outcome of `_isEmptyOrSignOnly(p_text) == true`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 80, cols 30-53 - Cover the true outcome in 2 instrumented instances of `(p_text.front() == '+')`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 84, cols 9-26 - Cover the true outcome and false outcome of `ec != std::errc()`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 84, cols 30-40 - Cover the true outcome and false outcome of `ptr != end`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 98, cols 9-30 - Cover the true outcome and false outcome of `stream.fail() == true`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 104, cols 9-30 - Cover the true outcome and false outcome of `stream.eof() == false`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 128, cols 8-33 - Cover the false outcome of `_isRefreshingText == true`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 134, cols 8-83 - Cover the true outcome in 2 instrumented instances and false outcome in 2 instrumented instances of `_parseValue(_valueEdit.textAsUTF8(), parsedValue) == ValidationState::Valid`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 145, cols 5-42 - Cover the false outcome in 3 instrumented instances of `(geometry().width() > buttonSize * 2)`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 167, cols 30-31 - Cover the true outcome and false outcome of `:`.
- [ ] `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 169, cols 11-26 - Cover the true outcome in 3 instrumented instances and false outcome of `codepoint > 127`.

### includes/structures/widget/spk_spin_box.hpp

Coverage: lines 93.10% (108/116, missing 8); branches 75.00% (6/8, missing 2); functions 95.00% (19/20, missing 1); regions 94.12% (32/34, missing 2).

Missing functions or function instantiations:

- [ ] `includes/structures/widget/spk_spin_box.hpp` - Lines 56-64 - Execute the currently uncalled function: `void _onGeometryChange() override`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Lines 78-80 - Execute the currently uncalled function: `_downButtonContract = _downButton.subscribeToClick([this]() {`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Lines 82-84 - Execute the currently uncalled function: `_upButtonContract = _upButton.subscribeToClick([this]() {`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Lines 100-108 - Execute the currently uncalled function: `configureMinimalSizeGenerator([this]() {`. All 2 instrumented functions/instantiations at this location are unexecuted.

Uncovered code regions:

- [ ] `includes/structures/widget/spk_spin_box.hpp` - Lines 44-46 - Exercise this uncovered code path: `p_value = std::max(p_value, _minLimit.value());`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Lines 48-50 - Exercise this uncovered code path: `p_value = std::min(p_value, _maxLimit.value());`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Line 59 - Exercise this uncovered code path: `0`.

Uncovered branch outcomes:

- [ ] `includes/structures/widget/spk_spin_box.hpp` - Line 43, cols 8-37 - Cover the true outcome of `_minLimit.has_value() == true`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Line 47, cols 8-37 - Cover the true outcome of `_maxLimit.has_value() == true`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Line 59, cols 5-42 - Cover the true outcome and false outcome in 2 instrumented instances of `(geometry().width() > buttonSize * 2)`.
- [ ] `includes/structures/widget/spk_spin_box.hpp` - Line 92, cols 8-26 - Cover the false outcome in 2 instrumented instances of `iconset != nullptr`.

### includes/structures/widget/spk_widget.hpp

Coverage: lines 100.00% (19/19, missing 0); branches 80.00% (8/10, missing 2); functions 100.00% (1/1, missing 0); regions 100.00% (12/12, missing 0).

Uncovered code regions:

- [ ] `includes/structures/widget/spk_widget.hpp` - Lines 65-67 - Exercise this uncovered code path: `return;`. This region is uncovered in 11 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_widget.hpp` - Lines 70-79 - Exercise this uncovered code path: `if (child != nullptr) child->_propagate(p_event, p_handler); if (p_event.isConsumed() == true)`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_widget.hpp` - Line 71 - Exercise this uncovered code path: `child != nullptr`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_widget.hpp` - Lines 72-78 - Exercise this uncovered code path: `child->_propagate(p_event, p_handler); if (p_event.isConsumed() == true) return;`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_widget.hpp` - Line 74 - Exercise this uncovered code path: `p_event.isConsumed() == true`. This region is uncovered in 7 instrumented functions/instantiations.
- [ ] `includes/structures/widget/spk_widget.hpp` - Lines 75-77 - Exercise this uncovered code path: `return;`. This region is uncovered in 15 instrumented functions/instantiations.

Uncovered branch outcomes:

- [ ] `includes/structures/widget/spk_widget.hpp` - Line 64, cols 8-30 - Cover the true outcome in 11 instrumented instances of `isActivated() == false`.
- [ ] `includes/structures/widget/spk_widget.hpp` - Line 64, cols 34-62 - Cover the true outcome in 18 instrumented instances of `p_event.isConsumed() == true`.
- [ ] `includes/structures/widget/spk_widget.hpp` - Line 69, cols 21-22 - Cover the true outcome in 7 instrumented instances of `:`.
- [ ] `includes/structures/widget/spk_widget.hpp` - Line 71, cols 9-25 - Cover the true outcome in 7 instrumented instances and false outcome in 18 instrumented instances of `child != nullptr`.
- [ ] `includes/structures/widget/spk_widget.hpp` - Line 74, cols 10-38 - Cover the true outcome in 15 instrumented instances and false outcome in 8 instrumented instances of `p_event.isConsumed() == true`.

### includes/structures/widget/spk_workspace.hpp

Coverage: lines 100.00% (34/34, missing 0); branches 50.00% (1/2, missing 1); functions 100.00% (5/5, missing 0); regions 90.91% (10/11, missing 1).

Uncovered code regions:

- [ ] `includes/structures/widget/spk_workspace.hpp` - Line 26 - Exercise this uncovered code path: `0`.

Uncovered branch outcomes:

- [ ] `includes/structures/widget/spk_workspace.hpp` - Line 26, cols 5-46 - Cover the false outcome of `(geometry().height() > _menuBar.height())`.

### srcs/structures/application/module/spk_keyboard_module.cpp

Coverage: lines 93.33% (42/45, missing 3); branches 100.00% (4/4, missing 0); functions 88.89% (8/9, missing 1); regions 92.86% (13/14, missing 1).

Missing functions or function instantiations:

- [ ] `srcs/structures/application/module/spk_keyboard_module.cpp` - Lines 53-55 - Execute the currently uncalled function: `const spk::Keyboard &KeyboardModule::keyboard() const`.

### srcs/structures/application/module/spk_mouse_module.cpp

Coverage: lines 95.89% (70/73, missing 3); branches 100.00% (4/4, missing 0); functions 92.31% (12/13, missing 1); regions 94.44% (17/18, missing 1).

Missing functions or function instantiations:

- [ ] `srcs/structures/application/module/spk_mouse_module.cpp` - Lines 67-69 - Execute the currently uncalled function: `const spk::Mouse &MouseModule::mouse() const`.

### srcs/structures/application/module/spk_render_module.cpp

Coverage: lines 82.35% (14/17, missing 3); branches 50.00% (1/2, missing 1); functions 100.00% (4/4, missing 0); regions 85.71% (6/7, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/application/module/spk_render_module.cpp` - Lines 26-28 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/application/module/spk_render_module.cpp` - Line 25, cols 7-26 - Cover the true outcome of `snapshot == nullptr`.

### srcs/structures/application/spk_application.cpp

Coverage: lines 93.66% (251/268, missing 17); branches 82.89% (63/76, missing 13); functions 100.00% (21/21, missing 0); regions 94.78% (109/115, missing 6).

Uncovered code regions:

- [ ] `srcs/structures/application/spk_application.cpp` - Lines 85-87 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/application/spk_application.cpp` - Lines 129-131 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/application/spk_application.cpp` - Lines 176-177 - Exercise this uncovered code path: `spk::TimeUtils::sleepFor(_configuration.eventPollingInterval); continue;`.
- [ ] `srcs/structures/application/spk_application.cpp` - Lines 275-277 - Exercise this uncovered code path: `_platformRuntime = _createDefaultPlatformRuntime();`.
- [ ] `srcs/structures/application/spk_application.cpp` - Lines 280-282 - Exercise this uncovered code path: `_gpuPlatformRuntime = _createDefaultGPUPlatformRuntime();`.
- [ ] `srcs/structures/application/spk_application.cpp` - Lines 321-324 - Exercise this uncovered code path: `_recordFailure(std::current_exception()); _stopRequested.store(true);`.

Uncovered branch outcomes:

- [ ] `srcs/structures/application/spk_application.cpp` - Line 75, cols 6-46 - Cover the false outcome of `_configuration.stopWhenNoWindows == true`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 84, cols 10-39 - Cover the true outcome of `_stopRequested.load() == true`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 90, cols 10-27 - Cover the false outcome of `window != nullptr`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 119, cols 6-46 - Cover the false outcome of `_configuration.stopWhenNoWindows == true`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 128, cols 10-39 - Cover the true outcome of `_stopRequested.load() == true`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 134, cols 10-27 - Cover the false outcome of `window != nullptr`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 161, cols 10-27 - Cover the false outcome of `window != nullptr`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 170, cols 9-42 - Cover the true outcome of `_shutdownRequested.load() == true`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 171, cols 6-46 - Cover the false outcome of `_configuration.stopWhenNoWindows == true`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 186, cols 9-26 - Cover the false outcome of `window != nullptr`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 195, cols 43-83 - Cover the false outcome of `_configuration.stopWhenNoWindows == true`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 274, cols 38-65 - Cover the true outcome of `_windowRegistry.size() != 0`.
- [ ] `srcs/structures/application/spk_application.cpp` - Line 279, cols 41-68 - Cover the true outcome of `_windowRegistry.size() != 0`.

### srcs/structures/container/spk_binary_field.cpp

Coverage: lines 98.31% (174/177, missing 3); branches 95.83% (46/48, missing 2); functions 100.00% (21/21, missing 0); regions 98.85% (86/87, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/container/spk_binary_field.cpp` - Lines 16-18 - Exercise this uncovered code path: `throw std::runtime_error("Invalid BinaryField.");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/container/spk_binary_field.cpp` - Line 15, cols 7-25 - Cover the true outcome of `isValid() == false`.
- [ ] `srcs/structures/container/spk_binary_field.cpp` - Line 123, cols 32-69 - Cover the false outcome of `_sectionID < _layout->sections.size()`.

### srcs/structures/design_pattern/spk_blockable_trait.cpp

Coverage: lines 86.73% (98/113, missing 15); branches 77.78% (28/36, missing 8); functions 100.00% (14/14, missing 0); regions 91.80% (56/61, missing 5).

Uncovered code regions:

- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 22-24 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 55-57 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 131-133 - Exercise this uncovered code path: `return false;`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 141-143 - Exercise this uncovered code path: `return false;`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 151-153 - Exercise this uncovered code path: `return false;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 13, cols 7-24 - Cover the false outcome of `_state != nullptr`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 21, cols 7-24 - Cover the true outcome of `_state == nullptr`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 54, cols 7-23 - Cover the true outcome of `state == nullptr`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 99, cols 9-33 - Cover the false outcome of `state->nbDelayBlocks > 0`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 106, cols 9-34 - Cover the false outcome of `state->nbIgnoreBlocks > 0`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 130, cols 7-24 - Cover the true outcome of `_state == nullptr`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 140, cols 7-24 - Cover the true outcome of `_state == nullptr`.
- [ ] `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 150, cols 7-24 - Cover the true outcome of `_state == nullptr`.

### srcs/structures/design_pattern/spk_synchronizable_trait.cpp

Coverage: lines 89.29% (25/28, missing 3); branches 75.00% (3/4, missing 1); functions 100.00% (4/4, missing 0); regions 91.67% (11/12, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/design_pattern/spk_synchronizable_trait.cpp` - Lines 23-25 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/design_pattern/spk_synchronizable_trait.cpp` - Line 22, cols 7-53 - Cover the true outcome of `_needsSynchronization.exchange(false) == false`.

### srcs/structures/game_engine/spk_camera_2d.cpp

Coverage: lines 0.00% (0/57, missing 57); branches 0.00% (0/12, missing 12); functions 0.00% (0/12, missing 12); regions 0.00% (0/27, missing 27).

Missing functions or function instantiations:

- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 12 - Execute the currently uncalled function: `_projectionMatrix([this]() { return _generateProjectionMatrix(); })`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 12 - Execute the currently uncalled function: `_projectionMatrix([this]() { return _generateProjectionMatrix(); })`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 17-22 - Execute the currently uncalled function: `Camera2D::~Camera2D()`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 25-27 - Execute the currently uncalled function: `Camera2D *Camera2D::mainCamera()`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 30-32 - Execute the currently uncalled function: `void Camera2D::makeMain()`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 35-38 - Execute the currently uncalled function: `void Camera2D::setViewport(const spk::Rect2D &p_viewport)`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 41-43 - Execute the currently uncalled function: `const spk::Rect2D &Camera2D::viewport() const`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 46-49 - Execute the currently uncalled function: `void Camera2D::setPixelsPerUnit(const spk::Vector2 &p_pixelsPerUnit)`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 52-54 - Execute the currently uncalled function: `const spk::Vector2 &Camera2D::pixelsPerUnit() const`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 57-76 - Execute the currently uncalled function: `spk::Matrix4x4 Camera2D::_generateProjectionMatrix() const`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 79-81 - Execute the currently uncalled function: `const spk::Matrix4x4 &Camera2D::projectionMatrix() const`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Lines 84-91 - Execute the currently uncalled function: `spk::Matrix4x4 Camera2D::viewMatrix() const`.

Uncovered branch outcomes:

- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 18, cols 7-26 - Cover the true outcome and false outcome of `_mainCamera == this`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 61, cols 7-20 - Cover the true outcome and false outcome of `width <= 0.0f`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 61, cols 24-38 - Cover the true outcome and false outcome of `height <= 0.0f`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 61, cols 42-66 - Cover the true outcome and false outcome of `_pixelsPerUnit.x <= 0.0f`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 61, cols 70-94 - Cover the true outcome and false outcome of `_pixelsPerUnit.y <= 0.0f`.
- [ ] `srcs/structures/game_engine/spk_camera_2d.cpp` - Line 85, cols 7-26 - Cover the true outcome and false outcome of `entity() != nullptr`.

### srcs/structures/game_engine/spk_component.cpp

Coverage: lines 100.00% (29/29, missing 0); branches 87.50% (7/8, missing 1); functions 100.00% (9/9, missing 0); regions 100.00% (15/15, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/game_engine/spk_component.cpp` - Line 21, cols 7-26 - Cover the false outcome of `p_entity != nullptr`.

### srcs/structures/game_engine/spk_component_2d.cpp

Coverage: lines 0.00% (0/12, missing 12); branches 0.00% (0/2, missing 2); functions 0.00% (0/3, missing 3); regions 0.00% (0/5, missing 5).

Missing functions or function instantiations:

- [ ] `srcs/structures/game_engine/spk_component_2d.cpp` - Lines 11-16 - Execute the currently uncalled function: `void Component2D::_onAttached(spk::Entity &p_entity)`.
- [ ] `srcs/structures/game_engine/spk_component_2d.cpp` - Lines 19-21 - Execute the currently uncalled function: `Entity2D *Component2D::entity()`.
- [ ] `srcs/structures/game_engine/spk_component_2d.cpp` - Lines 24-26 - Execute the currently uncalled function: `const Entity2D *Component2D::entity() const`.

Uncovered branch outcomes:

- [ ] `srcs/structures/game_engine/spk_component_2d.cpp` - Line 12, cols 7-53 - Cover the true outcome and false outcome of `dynamic_cast<Entity2D *>(&p_entity) == nullptr`.

### srcs/structures/game_engine/spk_component_store.cpp

Coverage: lines 86.00% (43/50, missing 7); branches 87.50% (14/16, missing 2); functions 100.00% (6/6, missing 0); regions 92.59% (25/27, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/game_engine/spk_component_store.cpp` - Lines 41-43 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/game_engine/spk_component_store.cpp` - Lines 60-62 - Exercise this uncovered code path: `++iterator;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/game_engine/spk_component_store.cpp` - Line 40, cols 7-21 - Cover the true outcome of `p_from == p_to`.
- [ ] `srcs/structures/game_engine/spk_component_store.cpp` - Line 55, cols 8-40 - Cover the false outcome of `iterator->second.empty() == true`.

### srcs/structures/game_engine/spk_entity.cpp

Coverage: lines 94.74% (126/133, missing 7); branches 84.09% (37/44, missing 7); functions 94.12% (16/17, missing 1); regions 92.75% (64/69, missing 5).

Missing functions or function instantiations:

- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 36 - Execute the currently uncalled function: `_deactivationContract = subscribeToDeactivation([this]() { _refreshGlobalActivated(); });`.

Uncovered code regions:

- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 33 - Exercise this uncovered code path: `parent()->_engineId`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Lines 70-72 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Lines 84-86 - Exercise this uncovered code path: `continue;`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 117 - Exercise this uncovered code path: `spk::UUID::null()`.

Uncovered branch outcomes:

- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 33, cols 16-35 - Cover the true outcome of `parent() != nullptr`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 60, cols 8-24 - Cover the false outcome of `child != nullptr`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 69, cols 7-30 - Cover the true outcome of `_engineId == p_engineId`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 83, cols 8-31 - Cover the true outcome of `componentPtr == nullptr`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 98, cols 13-41 - Cover the false outcome of `p_engineId.isNull() == false`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 106, cols 8-24 - Cover the false outcome of `child != nullptr`.
- [ ] `srcs/structures/game_engine/spk_entity.cpp` - Line 117, cols 16-38 - Cover the false outcome of `p_newParent != nullptr`.

### srcs/structures/game_engine/spk_game_engine.cpp

Coverage: lines 88.00% (44/50, missing 6); branches 80.00% (8/10, missing 2); functions 90.91% (10/11, missing 1); regions 91.67% (22/24, missing 2).

Missing functions or function instantiations:

- [ ] `srcs/structures/game_engine/spk_game_engine.cpp` - Lines 18-20 - Execute the currently uncalled function: `const spk::UUID &GameEngine::id() const`.

Uncovered code regions:

- [ ] `srcs/structures/game_engine/spk_game_engine.cpp` - Lines 45-47 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/game_engine/spk_game_engine.cpp` - Line 44, cols 7-26 - Cover the true outcome of `p_entity == nullptr`.
- [ ] `srcs/structures/game_engine/spk_game_engine.cpp` - Line 54, cols 7-26 - Cover the true outcome of `p_entity == nullptr`.

### srcs/structures/game_engine/spk_transform_2d.cpp

Coverage: lines 0.00% (0/101, missing 101); branches 0.00% (0/24, missing 24); functions 0.00% (0/15, missing 15); regions 0.00% (0/48, missing 48).

Missing functions or function instantiations:

- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 11 - Execute the currently uncalled function: `Transform2D::Transform2D() :`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 11 - Execute the currently uncalled function: `Transform2D::Transform2D() :`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 12 - Execute the currently uncalled function: `_inverseModelTransform([this]() { return _generateInverseModelTransform(); })`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 17-34 - Execute the currently uncalled function: `void Transform2D::_notifyDescendantModelTransformEditions(spk::Entity &p_entity)`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 37-46 - Execute the currently uncalled function: `void Transform2D::_notifyModelTransformEdition()`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 49-57 - Execute the currently uncalled function: `void Transform2D::setPosition(const spk::Vector2 &p_position)`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 60-62 - Execute the currently uncalled function: `void Transform2D::move(const spk::Vector2 &p_delta)`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 65-73 - Execute the currently uncalled function: `void Transform2D::setRotation(float p_degrees)`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 76-84 - Execute the currently uncalled function: `void Transform2D::setScale(const spk::Vector2 &p_scale)`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 87-95 - Execute the currently uncalled function: `void Transform2D::setLayer(float p_layer)`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 98-123 - Execute the currently uncalled function: `spk::Matrix4x4 Transform2D::_generateModelTransform() const`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 126-128 - Execute the currently uncalled function: `spk::Matrix4x4 Transform2D::_generateInverseModelTransform() const`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 131-133 - Execute the currently uncalled function: `const spk::Matrix4x4 &Transform2D::modelTransform() const`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 136-138 - Execute the currently uncalled function: `const spk::Matrix4x4 &Transform2D::inverseModelTransform() const`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Lines 141-143 - Execute the currently uncalled function: `Transform2D::EditionContract Transform2D::subscribeToModelTransformEdition(EditionCallback p_callback)`.

Uncovered branch outcomes:

- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 18, cols 27-28 - Cover the true outcome and false outcome of `:`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 20, cols 8-24 - Cover the true outcome and false outcome of `child == nullptr`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 25, cols 66-86 - Cover the true outcome and false outcome of `transform != nullptr`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 42, cols 38-54 - Cover the true outcome and false outcome of `owner != nullptr`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 50, cols 7-30 - Cover the true outcome and false outcome of `_position == p_position`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 66, cols 7-29 - Cover the true outcome and false outcome of `_rotation == p_degrees`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 77, cols 7-24 - Cover the true outcome and false outcome of `_scale == p_scale`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 88, cols 7-24 - Cover the true outcome and false outcome of `_layer == p_layer`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 106, cols 39-55 - Cover the true outcome and false outcome of `owner != nullptr`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 107, cols 5-24 - Cover the true outcome and false outcome of `ancestor != nullptr`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 111, cols 8-34 - Cover the true outcome and false outcome of `parentTransform != nullptr`.
- [ ] `srcs/structures/game_engine/spk_transform_2d.cpp` - Line 117, cols 7-33 - Cover the true outcome and false outcome of `parentTransform != nullptr`.

### srcs/structures/graphics/opengl/spk_opengl_buffer.cpp

Coverage: lines 100.00% (36/36, missing 0); branches 87.50% (7/8, missing 1); functions 100.00% (5/5, missing 0); regions 100.00% (26/26, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer.cpp` - Line 17, cols 8-16 - Cover the false outcome of `_id != 0`.

### srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp

Coverage: lines 96.76% (179/185, missing 6); branches 82.50% (33/40, missing 7); functions 100.00% (31/31, missing 0); regions 96.47% (82/85, missing 3).

Uncovered code regions:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 80 - Exercise this uncovered code path: `nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Lines 146-148 - Exercise this uncovered code path: `offset += p_alignment - remainder;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Lines 251-253 - Exercise this uncovered code path: `synchronize();`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 33, cols 7-21 - Cover the false outcome of `ctx != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 33, cols 25-62 - Cover the false outcome of `ctx->supportsOpenGLCommands() == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 80, cols 6-32 - Cover the true outcome of `_cpuBuffer.empty() == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 90, cols 7-45 - Cover the false outcome of `object->version() == _structureVersion`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 145, cols 8-22 - Cover the true outcome of `remainder != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 240, cols 7-37 - Cover the false outcome of `needsSynchronization() == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 250, cols 7-37 - Cover the true outcome of `needsSynchronization() == true`.

### srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp

Coverage: lines 74.14% (43/58, missing 15); branches 50.00% (7/14, missing 7); functions 66.67% (4/6, missing 2); regions 83.58% (56/67, missing 11).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Lines 82-84 - Execute the currently uncalled function: `void FrameBufferObject::bind() const`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Lines 87-89 - Execute the currently uncalled function: `void FrameBufferObject::bindDefault()`.

Uncovered code regions:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Lines 15-17 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Lines 21-23 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Lines 56-58 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Line 14, cols 8-19 - Cover the true outcome of `size.x == 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Line 14, cols 23-34 - Cover the true outcome of `size.y == 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Line 20, cols 8-27 - Cover the true outcome of `colorTextureId == 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Line 30, cols 8-34 - Cover the false outcome of `p_owner.hasDepth() == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Line 55, cols 8-38 - Cover the true outcome of `_ownsCurrentContext() == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Line 60, cols 8-27 - Cover the false outcome of `_framebufferId != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp` - Line 65, cols 8-38 - Cover the false outcome of `_depthStencilRenderbuffer != 0`.

### srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp

Coverage: lines 89.59% (198/221, missing 23); branches 80.43% (74/92, missing 18); functions 96.30% (26/27, missing 1); regions 90.99% (101/111, missing 10).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Lines 80-92 - Execute the currently uncalled function: `LayoutBufferObject &LayoutBufferObject::operator=(const LayoutBufferObject &p_other)`.

Uncovered code regions:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 35 - Exercise this uncovered code path: `throw std::runtime_error("spk::LayoutBufferObject::Attribute has an unsupported component count")`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 58 - Exercise this uncovered code path: `throw std::runtime_error("spk::LayoutBufferObject::Attribute has an unsupported component type")`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 208 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 213 - Exercise this uncovered code path: `p_size != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Lines 214-216 - Exercise this uncovered code path: `throw std::runtime_error("spk::LayoutBufferObject requires at least one attribute before vertex upload");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Lines 218-222 - Exercise this uncovered code path: `throw std::runtime_error( "spk::LayoutBufferObject vertex data size [" + std::to_string(p_size) + "] is not aligned with its vertex layout [" + std::to_string(_vertexSize) + "]");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 228 - Exercise this uncovered code path: `0`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 16, cols 11-17 - Cover the false outcome of `p_type`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 23, cols 3-24 - Cover the true outcome of `case Type::Vector2Int`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 24, cols 3-25 - Cover the true outcome of `case Type::Vector2UInt`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 27, cols 3-24 - Cover the true outcome of `case Type::Vector3Int`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 28, cols 3-25 - Cover the true outcome of `case Type::Vector3UInt`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 40, cols 11-17 - Cover the false outcome of `p_type`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 81, cols 7-23 - Cover the true outcome and false outcome of `this != &p_other`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 192, cols 27-38 - Cover the false outcome of `p_size != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 196, cols 7-23 - Cover the false outcome of `_vertexSize != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 208, cols 18-34 - Cover the true outcome of `_vertexSize == 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 213, cols 7-23 - Cover the true outcome of `_vertexSize == 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 213, cols 27-38 - Cover the true outcome and false outcome of `p_size != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 217, cols 7-23 - Cover the false outcome of `_vertexSize != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 217, cols 27-52 - Cover the true outcome of `p_size % _vertexSize != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 224, cols 7-18 - Cover the false outcome of `p_size != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 228, cols 18-34 - Cover the true outcome of `_vertexSize == 0`.

### srcs/structures/graphics/opengl/spk_opengl_program.cpp

Coverage: lines 79.68% (149/187, missing 38); branches 50.00% (18/36, missing 18); functions 88.46% (23/26, missing 3); regions 79.67% (98/123, missing 25).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 108-121 - Execute the currently uncalled function: `void Program::deactivate() const`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 143-151 - Execute the currently uncalled function: `void Program::renderInstanced(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount, std::size_t p_instanceCount) const`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 260-263 - Execute the currently uncalled function: `void Program::renderInstanced(const spk::RenderContext &p_context, spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount, std::size_t p_instanceCount) const`.

Uncovered code regions:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 16-18 - Exercise this uncovered code path: `throw std::runtime_error("spk::Program requires a current spk::RenderContext to resolve its compiled program");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 43-50 - Exercise this uncovered code path: `GLint logLength = 0; glGetProgramiv(newProgram, GL_INFO_LOG_LENGTH, &logLength); std::string log(static_cast<std::size_t>(logLength), '\0');`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 45 - Exercise this uncovered code path: `g`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 45 - Exercise this uncovered code path: `G`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 47 - Exercise this uncovered code path: `g`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 48 - Exercise this uncovered code path: `g`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 235-237 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 15, cols 7-25 - Cover the true outcome of `context == nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 42, cols 8-25 - Cover the true outcome of `status != GL_TRUE`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 57, cols 8-16 - Cover the false outcome of `_id != 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 94, cols 8-26 - Cover the false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 101, cols 8-26 - Cover the false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 110, cols 8-26 - Cover the true outcome and false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 110, cols 30-71 - Cover the true outcome and false outcome of `context->isProgramActive(nullptr) == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 117, cols 8-26 - Cover the true outcome and false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 187, cols 10-27 - Cover the false outcome of `object != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 187, cols 31-61 - Cover the false outcome of `object->version() == version()`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 222, cols 10-28 - Cover the false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 222, cols 32-56 - Cover the false outcome of `hasGpu(*context) == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 234, cols 7-25 - Cover the false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 234, cols 29-70 - Cover the true outcome of `context->isProgramActive(nullptr) == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 241, cols 7-25 - Cover the false outcome of `context != nullptr`.

### srcs/structures/graphics/opengl/spk_opengl_render_context.cpp

Coverage: lines 81.79% (229/280, missing 51); branches 72.34% (68/94, missing 26); functions 100.00% (32/32, missing 0); regions 87.93% (153/174, missing 21).

Uncovered code regions:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 46-48 - Exercise this uncovered code path: `spk::throwLastError("ChoosePixelFormat");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 51-53 - Exercise this uncovered code path: `spk::throwLastError("SetPixelFormat");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 85-87 - Exercise this uncovered code path: `throw std::invalid_argument("spk::RenderContext requires a valid surface state");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 89-91 - Exercise this uncovered code path: `throw std::runtime_error("spk::RenderContext requires a valid WinAPI frame window");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 93-95 - Exercise this uncovered code path: `spk::throwLastError("GetDC");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 101-103 - Exercise this uncovered code path: `spk::throwLastError("wglCreateContext");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 110-114 - Exercise this uncovered code path: `throw std::runtime_error( "spk::RenderContext failed to initialize GLEW: " + std::string(reinterpret_cast<const char *>(glewGetErrorString(glewResult))));`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 160-163 - Exercise this uncovered code path: `wglMakeCurrent(previousDeviceContext, previousRenderContext); s_current = previousCurrent != this ? previousCurrent : nullptr;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 162 - Exercise this uncovered code path: `previousCurrent != this`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 162 - Exercise this uncovered code path: `previousCurrent`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 162 - Exercise this uncovered code path: `nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 175-177 - Exercise this uncovered code path: `s_current = nullptr;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 211-213 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 263-265 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 292-294 - Exercise this uncovered code path: `_bindingCache.uniformBuffers[i] = nullptr;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 339-348 - Exercise this uncovered code path: `if (IsWindow(_windowHandle) == FALSE) _valid = false; s_current = nullptr;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 340 - Exercise this uncovered code path: `IsWindow(_windowHandle) == FALSE`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 340 - Exercise this uncovered code path: `F`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 341-345 - Exercise this uncovered code path: `_valid = false; s_current = nullptr; return;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 347-348 - Exercise this uncovered code path: `spk::throwLastError("wglMakeCurrent");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 393-395 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 45, cols 7-23 - Cover the true outcome of `pixelFormat == 0`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 50, cols 7-73 - Cover the true outcome of `SetPixelFormat(p_deviceContext, pixelFormat, &descriptor) == FALSE`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 84, cols 7-31 - Cover the true outcome of `_surfaceState == nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 88, cols 7-31 - Cover the true outcome of `_windowHandle == nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 92, cols 7-32 - Cover the true outcome of `_deviceContext == nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 100, cols 7-32 - Cover the true outcome of `_renderContext == nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 109, cols 7-28 - Cover the true outcome of `glewResult != GLEW_OK`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 159, cols 44-83 - Cover the true outcome of `previousRenderContext != _renderContext`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 162, cols 17-40 - Cover the true outcome and false outcome of `previousCurrent != this`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 174, cols 7-24 - Cover the true outcome of `s_current == this`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 179, cols 35-60 - Cover the false outcome of `_deviceContext != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 210, cols 7-26 - Cover the true outcome of `p_object == nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 253, cols 10-68 - Cover the false outcome of `p_bindingPoint < BindingCache::TrackedUniformBindingPoints`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 255, cols 7-71 - Cover the false outcome of `_bindingCache.uniformBuffers[p_bindingPoint].value() == p_buffer`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 262, cols 7-66 - Cover the true outcome of `p_bindingPoint >= BindingCache::TrackedUniformBindingPoints`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 291, cols 5-57 - Cover the true outcome of `_bindingCache.uniformBuffers[i].value() == &p_buffer`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 312, cols 7-31 - Cover the false outcome of `_surfaceState != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 321, cols 7-31 - Cover the false outcome of `_windowHandle != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 322, cols 7-32 - Cover the false outcome of `_deviceContext != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 323, cols 7-32 - Cover the false outcome of `_renderContext != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 338, cols 7-62 - Cover the true outcome of `wglMakeCurrent(_deviceContext, _renderContext) == FALSE`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 340, cols 8-40 - Cover the true outcome and false outcome of `IsWindow(_windowHandle) == FALSE`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 375, cols 7-30 - Cover the false outcome of `swapInterval != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 392, cols 7-25 - Cover the true outcome of `isValid() == false`.

### srcs/structures/graphics/opengl/spk_opengl_shader_storage_buffer_object.cpp

Coverage: lines 100.00% (79/79, missing 0); branches 94.44% (17/18, missing 1); functions 100.00% (14/14, missing 0); regions 100.00% (42/42, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_shader_storage_buffer_object.cpp` - Line 30, cols 7-33 - Cover the true outcome of `p_totalSize < _arrayOffset`.

### srcs/structures/graphics/opengl/spk_opengl_texture.cpp

Coverage: lines 86.29% (107/124, missing 17); branches 70.97% (44/62, missing 18); functions 83.33% (5/6, missing 1); regions 92.38% (97/105, missing 8).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Lines 8-24 - Execute the currently uncalled function: `size_t Texture::_bytesPerPixel(spk::Texture::Format p_format)`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 11, cols 4-40 - Cover the true outcome and false outcome of `case spk::Texture::Format::GreyLevel`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 13, cols 4-42 - Cover the true outcome and false outcome of `case spk::Texture::Format::DualChannel`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 15, cols 4-34 - Cover the true outcome and false outcome of `case spk::Texture::Format::RGB`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 16, cols 4-34 - Cover the true outcome and false outcome of `case spk::Texture::Format::BGR`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 18, cols 4-35 - Cover the true outcome and false outcome of `case spk::Texture::Format::RGBA`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 19, cols 4-35 - Cover the true outcome and false outcome of `case spk::Texture::Format::BGRA`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 21, cols 4-11 - Cover the true outcome and false outcome of `default`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 63, cols 12-18 - Cover the false outcome of `p_wrap`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 79, cols 12-23 - Cover the false outcome of `p_filtering`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 116, cols 37-62 - Cover the true outcome of `externalFormat == GL_NONE`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 139, cols 20-49 - Cover the false outcome of `_ownsCurrentContext() == true`.

### srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp

Coverage: lines 80.68% (142/176, missing 34); branches 56.90% (33/58, missing 25); functions 100.00% (13/13, missing 0); regions 78.31% (65/83, missing 18).

Uncovered code regions:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 57-59 - Exercise this uncovered code path: `throw std::runtime_error("spk::VertexArrayObject contains a null vertex buffer binding");`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 69 - Exercise this uncovered code path: `G`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 138-141 - Exercise this uncovered code path: `childrenClean = false; break;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 144-146 - Exercise this uncovered code path: `childrenClean = false;`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 149-166 - Exercise this uncovered code path: `if (p_context.isVertexArrayActive(nullptr) == false) glBindVertexArray(0); p_context.setActiveVertexArray(nullptr);`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 150 - Exercise this uncovered code path: `p_context.isVertexArrayActive(nullptr) == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 151-154 - Exercise this uncovered code path: `glBindVertexArray(0); p_context.setActiveVertexArray(nullptr);`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 152 - Exercise this uncovered code path: `g`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 156-161 - Exercise this uncovered code path: `if (binding.buffer != nullptr) (void)binding.buffer->gpu(p_context);`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 157 - Exercise this uncovered code path: `binding.buffer != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 158-160 - Exercise this uncovered code path: `(void)binding.buffer->gpu(p_context);`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 162 - Exercise this uncovered code path: `_indexBuffer != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 163-165 - Exercise this uncovered code path: `(void)_indexBuffer->gpu(p_context);`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 170-173 - Exercise this uncovered code path: `glBindVertexArray(vertexArray.id()); p_context.setActiveVertexArray(&vertexArray);`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 171 - Exercise this uncovered code path: `g`. This region is uncovered in 2 instrumented functions/instantiations.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 180-182 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 20, cols 8-33 - Cover the false outcome of `binding.buffer != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 35, cols 7-21 - Cover the false outcome of `ctx != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 35, cols 25-62 - Cover the false outcome of `ctx->supportsOpenGLCommands() == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 48, cols 9-66 - Cover the false outcome of `p_context.isVertexArrayActive(vertexArray.get()) == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 56, cols 10-35 - Cover the true outcome of `binding.buffer == nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 69, cols 7-35 - Cover the true outcome of `attribute.normalized == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 86, cols 10-27 - Cover the false outcome of `object != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 86, cols 31-71 - Cover the false outcome of `object->version() == _effectiveVersion()`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 137, cols 8-33 - Cover the false outcome of `binding.buffer != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 137, cols 37-79 - Cover the true outcome of `binding.buffer->hasGpu(p_context) == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 143, cols 7-28 - Cover the false outcome of `childrenClean == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 143, cols 59-99 - Cover the true outcome of `_indexBuffer->hasGpu(p_context) == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 148, cols 7-29 - Cover the true outcome of `childrenClean == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 150, cols 8-55 - Cover the true outcome and false outcome of `p_context.isVertexArrayActive(nullptr) == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 155, cols 44-45 - Cover the true outcome and false outcome of `:`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 157, cols 9-34 - Cover the true outcome and false outcome of `binding.buffer != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 162, cols 8-31 - Cover the true outcome and false outcome of `_indexBuffer != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 169, cols 7-59 - Cover the true outcome of `p_context.isVertexArrayActive(&vertexArray) == false`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 179, cols 7-25 - Cover the false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 179, cols 29-74 - Cover the true outcome of `context->isVertexArrayActive(nullptr) == true`.
- [ ] `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 186, cols 7-25 - Cover the false outcome of `context != nullptr`.

### srcs/structures/graphics/rendering/command/spk_camera_update_render_command.cpp

Coverage: lines 0.00% (0/8, missing 8); branches - (0/0, missing 0); functions 0.00% (0/2, missing 2); regions 0.00% (0/4, missing 4).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/rendering/command/spk_camera_update_render_command.cpp` - Line 8 - Execute the currently uncalled function: `CameraUpdateRenderCommand::CameraUpdateRenderCommand(GLuint p_bindingPoint, const spk::Matrix4x4 &p_viewProjection) :`.
- [ ] `srcs/structures/graphics/rendering/command/spk_camera_update_render_command.cpp` - Lines 14-17 - Execute the currently uncalled function: `void CameraUpdateRenderCommand::execute(spk::RenderContext &p_renderContext)`.

### srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp

Coverage: lines 76.92% (20/26, missing 6); branches 50.00% (2/4, missing 2); functions 100.00% (3/3, missing 0); regions 84.62% (11/13, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Lines 34-36 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Lines 40-42 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Line 33, cols 7-23 - Cover the true outcome of `_mesh == nullptr`.
- [ ] `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Line 39, cols 7-37 - Cover the true outcome of `layoutBuffer.indexCount() == 0`.

### srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp

Coverage: lines 0.00% (0/52, missing 52); branches 0.00% (0/2, missing 2); functions 0.00% (0/10, missing 10); regions 0.00% (0/20, missing 20).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 10-15 - Execute the currently uncalled function: `spk::Program &InstancedSpriteRenderCommand::_program()`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Line 18 - Execute the currently uncalled function: `_data(&p_data)`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 23-25 - Execute the currently uncalled function: `void InstancedSpriteRenderCommand::Instance::setModelTransform(const spk::Matrix4x4 &p_modelTransform)`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 28-30 - Execute the currently uncalled function: `void InstancedSpriteRenderCommand::Instance::setSprite(const spk::SpriteSheet::Sprite &p_sprite)`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Line 35 - Execute the currently uncalled function: `InstancedSpriteRenderCommand::InstancedSpriteRenderCommand(`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 44-46 - Execute the currently uncalled function: `InstancedSpriteRenderCommand::Instance InstancedSpriteRenderCommand::pushBackInstance()`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 49-51 - Execute the currently uncalled function: `std::size_t InstancedSpriteRenderCommand::instanceCount() const`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 54-56 - Execute the currently uncalled function: `const spk::SpriteSheet &InstancedSpriteRenderCommand::spriteSheet() const`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 59-61 - Execute the currently uncalled function: `const spk::TextureMesh2D &InstancedSpriteRenderCommand::mesh() const`.
- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Lines 64-82 - Execute the currently uncalled function: `void InstancedSpriteRenderCommand::execute(spk::RenderContext &p_renderContext)`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp` - Line 66, cols 7-25 - Cover the true outcome and false outcome of `instanceCount == 0`.

### srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp

Coverage: lines 100.00% (57/57, missing 0); branches 85.71% (12/14, missing 2); functions 100.00% (4/4, missing 0); regions 100.00% (22/22, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp` - Line 26, cols 29-47 - Cover the true outcome of `p_cornerSize.y < 0`.
- [ ] `srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp` - Line 32, cols 4-74 - Cover the true outcome of `static_cast<std::uint32_t>(p_cornerSize.y) > p_screenRect.height() / 2`.

### srcs/structures/graphics/rendering/command/spk_text_render_command.cpp

Coverage: lines 94.74% (108/114, missing 6); branches 81.25% (26/32, missing 6); functions 100.00% (7/7, missing 0); regions 95.35% (41/43, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Lines 81-83 - Exercise this uncovered code path: `throw std::invalid_argument("TextRenderCommand font cannot be null");`.
- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Lines 87-89 - Exercise this uncovered code path: `throw std::runtime_error("TextRenderCommand font returned a null atlas");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 33, cols 29-46 - Cover the false outcome of `glyph.size.y != 0`.
- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 80, cols 7-23 - Cover the true outcome of `_font == nullptr`.
- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 86, cols 7-24 - Cover the true outcome of `_atlas == nullptr`.
- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 103, cols 11-31 - Cover the false outcome of `_horizontalAlignment`.
- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 117, cols 11-29 - Cover the false outcome of `_verticalAlignment`.
- [ ] `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 165, cols 39-62 - Cover the true outcome of `_fontCommand == nullptr`.

### srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp

Coverage: lines 12.50% (4/32, missing 28); branches 0.00% (0/6, missing 6); functions 50.00% (1/2, missing 1); regions 21.43% (3/14, missing 11).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp` - Lines 23-55 - Execute the currently uncalled function: `void UseFrameBufferRenderCommand::execute(spk::RenderContext &p_renderContext)`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp` - Line 24, cols 7-56 - Cover the true outcome and false outcome of `p_renderContext.supportsOpenGLCommands() == false`.
- [ ] `srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp` - Line 31, cols 7-25 - Cover the true outcome and false outcome of `_target != nullptr`.
- [ ] `srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp` - Line 41, cols 8-33 - Cover the true outcome and false outcome of `contextSurface == nullptr`.

### srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp

Coverage: lines 85.00% (17/20, missing 3); branches 75.00% (3/4, missing 1); functions 100.00% (2/2, missing 0); regions 88.89% (8/9, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp` - Lines 27-29 - Exercise this uncovered code path: `throw std::runtime_error("spk::ViewportCommand::execute() - render context has no surface state");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp` - Line 26, cols 7-30 - Cover the true outcome of `surfaceState == nullptr`.

### srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp

Coverage: lines 71.43% (15/21, missing 6); branches 25.00% (1/4, missing 3); functions 100.00% (5/5, missing 0); regions 62.50% (5/8, missing 3).

Uncovered code regions:

- [ ] `srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp` - Lines 13-18 - Exercise this uncovered code path: `if (command != nullptr) _commands.emplace_back(std::move(command));`.
- [ ] `srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp` - Line 14 - Exercise this uncovered code path: `command != nullptr`.
- [ ] `srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp` - Lines 15-17 - Exercise this uncovered code path: `_commands.emplace_back(std::move(command));`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp` - Line 12, cols 53-54 - Cover the true outcome of `:`.
- [ ] `srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp` - Line 14, cols 8-26 - Cover the true outcome and false outcome of `command != nullptr`.

### srcs/structures/graphics/spk_framebuffer_object.cpp

Coverage: lines 89.06% (57/64, missing 7); branches 57.14% (8/14, missing 6); functions 92.31% (12/13, missing 1); regions 81.48% (22/27, missing 5).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/spk_framebuffer_object.cpp` - Lines 31-37 - Execute the currently uncalled function: `void FrameBufferObject::_synchronize() const`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/spk_framebuffer_object.cpp` - Line 24, cols 23-35 - Cover the false outcome of `p_size.y > 0`.
- [ ] `srcs/structures/graphics/spk_framebuffer_object.cpp` - Line 33, cols 7-25 - Cover the true outcome and false outcome of `context != nullptr`.
- [ ] `srcs/structures/graphics/spk_framebuffer_object.cpp` - Line 33, cols 29-70 - Cover the true outcome and false outcome of `context->supportsOpenGLCommands() == true`.
- [ ] `srcs/structures/graphics/spk_framebuffer_object.cpp` - Line 94, cols 31-61 - Cover the false outcome of `object->version() == version()`.

### srcs/structures/graphics/spk_gpu_data_buffer_center.cpp

Coverage: lines 100.00% (46/46, missing 0); branches 87.50% (14/16, missing 2); functions 100.00% (6/6, missing 0); regions 100.00% (28/28, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/spk_gpu_data_buffer_center.cpp` - Line 41, cols 28-46 - Cover the true outcome of `*buffer == nullptr`.
- [ ] `srcs/structures/graphics/spk_gpu_data_buffer_center.cpp` - Line 58, cols 28-46 - Cover the true outcome of `*buffer == nullptr`.

### srcs/structures/graphics/spk_uniform.cpp

Coverage: lines 93.48% (43/46, missing 3); branches 83.33% (5/6, missing 1); functions 100.00% (3/3, missing 0); regions 92.31% (12/13, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/graphics/spk_uniform.cpp` - Lines 23-25 - Exercise this uncovered code path: `throw std::runtime_error("spk::UniformBase: no current render context");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/spk_uniform.cpp` - Line 22, cols 7-21 - Cover the true outcome of `raw == nullptr`.

### srcs/structures/graphics/texture/spk_font.cpp

Coverage: lines 96.89% (280/289, missing 9); branches 94.74% (72/76, missing 4); functions 93.33% (28/30, missing 2); regions 97.54% (119/122, missing 3).

Missing functions or function instantiations:

- [ ] `srcs/structures/graphics/texture/spk_font.cpp` - Lines 245-247 - Execute the currently uncalled function: `spk::Vector2UInt Font::Atlas::computeStringSize(std::string_view p_utf8String)`.
- [ ] `srcs/structures/graphics/texture/spk_font.cpp` - Lines 267-269 - Execute the currently uncalled function: `spk::Vector2Int Font::Atlas::computeStringBaselineOffset(std::string_view p_utf8String)`.

Uncovered code regions:

- [ ] `srcs/structures/graphics/texture/spk_font.cpp` - Lines 140-142 - Exercise this uncovered code path: `_resizeData(_resource->atlasSize * 2);`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/texture/spk_font.cpp` - Line 138, cols 10-107 - Cover the true outcome of `(p_glyphPosition.x + static_cast<int>(p_glyphSize.x) >= static_cast<int>(_resource->atlasSize.x))`.
- [ ] `srcs/structures/graphics/texture/spk_font.cpp` - Line 139, cols 7-104 - Cover the true outcome of `(p_glyphPosition.y + static_cast<int>(p_glyphSize.y) >= static_cast<int>(_resource->atlasSize.y))`.
- [ ] `srcs/structures/graphics/texture/spk_font.cpp` - Line 304, cols 8-29 - Cover the false outcome of `atlasEntry != nullptr`.
- [ ] `srcs/structures/graphics/texture/spk_font.cpp` - Line 392, cols 31-52 - Cover the true outcome of `it->second == nullptr`.

### srcs/structures/graphics/texture/spk_font_true_type.cpp

Coverage: lines 92.59% (150/162, missing 12); branches 84.09% (37/44, missing 7); functions 100.00% (7/7, missing 0); regions 91.67% (44/48, missing 4).

Uncovered code regions:

- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Lines 94-97 - Exercise this uncovered code path: `resource.currentQuadrant = Quadrant::DownRight; _resetToQuadrant(spk::Vector2Int(static_cast<int>(resource.atlasSize.x / 2), static_cast<int>(resource.atlasSize.y / 2)));`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Lines 100-107 - Exercise this uncovered code path: `case Quadrant::DownRight: if (_overflowQuadrant()) resource.currentQuadrant = Quadrant::TopRight;`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 101 - Exercise this uncovered code path: `_overflowQuadrant()`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Lines 102-106 - Exercise this uncovered code path: `resource.currentQuadrant = Quadrant::TopRight; _resizeData(resource.atlasSize * 2); _resetToQuadrant(spk::Vector2Int(static_cast<int>(resource.atlasSize.x / 2), 0));`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 25, cols 9-46 - Cover the false outcome of `rendered.contains(codepoint) == false`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 67, cols 8-78 - Cover the false outcome of `resource.nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y)`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 73, cols 11-35 - Cover the false outcome of `resource.currentQuadrant`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 93, cols 8-27 - Cover the true outcome of `_overflowQuadrant()`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 100, cols 3-27 - Cover the true outcome of `case Quadrant::DownRight`.
- [ ] `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 101, cols 8-27 - Cover the true outcome and false outcome of `_overflowQuadrant()`.

### srcs/structures/graphics/texture/spk_image.cpp

Coverage: lines 96.77% (60/62, missing 2); branches 92.86% (13/14, missing 1); functions 100.00% (5/5, missing 0); regions 93.75% (15/16, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/graphics/texture/spk_image.cpp` - Lines 85-86 - Exercise this uncovered code path: `default: return Format::Error;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/texture/spk_image.cpp` - Line 85, cols 3-10 - Cover the true outcome of `default`.

### srcs/structures/graphics/texture/spk_texture.cpp

Coverage: lines 98.10% (310/316, missing 6); branches 92.11% (70/76, missing 6); functions 100.00% (41/41, missing 0); regions 96.24% (128/133, missing 5).

Uncovered code regions:

- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Lines 43-45 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 114 - Exercise this uncovered code path: `std::make_shared<Resource>()`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 127 - Exercise this uncovered code path: `emptyResource`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Lines 133-135 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 141 - Exercise this uncovered code path: `0`.

Uncovered branch outcomes:

- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 42, cols 7-24 - Cover the true outcome of `p_id == InvalidID`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 114, cols 10-37 - Cover the false outcome of `_state->resource != nullptr`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 127, cols 10-37 - Cover the false outcome of `_state->resource != nullptr`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 132, cols 7-28 - Cover the true outcome of `p_resource == nullptr`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 141, cols 41-68 - Cover the false outcome of `_state->resource != nullptr`.
- [ ] `srcs/structures/graphics/texture/spk_texture.cpp` - Line 152, cols 25-62 - Cover the false outcome of `ctx->supportsOpenGLCommands() == true`.

### srcs/structures/system/device/window/spk_frame.cpp

Coverage: lines 93.18% (41/44, missing 3); branches 83.33% (5/6, missing 1); functions 90.91% (10/11, missing 1); regions 94.44% (17/18, missing 1).

Missing functions or function instantiations:

- [ ] `srcs/structures/system/device/window/spk_frame.cpp` - Lines 22-24 - Execute the currently uncalled function: `void IFrame::setCursor(const std::string &p_name)`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/device/window/spk_frame.cpp` - Line 48, cols 7-31 - Cover the false outcome of `_surfaceState != nullptr`.

### srcs/structures/system/device/window/spk_window.cpp

Coverage: lines 88.57% (310/350, missing 40); branches 79.81% (83/104, missing 21); functions 90.91% (40/44, missing 4); regions 90.41% (132/146, missing 14).

Missing functions or function instantiations:

- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 302-304 - Execute the currently uncalled function: `const spk::WindowHost &Window::host() const`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 312-314 - Execute the currently uncalled function: `const spk::Mouse &Window::mouse() const`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 322-324 - Execute the currently uncalled function: `const spk::Keyboard &Window::keyboard() const`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 332-334 - Execute the currently uncalled function: `const spk::Widget &Window::rootWidget() const`.

Uncovered code regions:

- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 15-17 - Exercise this uncovered code path: `throw std::runtime_error("spk::Window requires a valid spk::PlatformRuntime to create its frame");`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 22-24 - Exercise this uncovered code path: `throw std::runtime_error("spk::Window failed to create its frame");`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 149-154 - Exercise this uncovered code path: `case PlatformActionType::SetCursor: if (p_action.cursorShape.has_value() == true) _host.setCursor(p_action.cursorShape.value());`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 150 - Exercise this uncovered code path: `p_action.cursorShape.has_value() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 151-153 - Exercise this uncovered code path: `_host.setCursor(p_action.cursorShape.value());`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 185-187 - Exercise this uncovered code path: `return false;`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 200-203 - Exercise this uncovered code path: `_appliedCursorShape = requestedShape; _enqueuePlatformAction(PlatformAction{.type = PlatformActionType::SetCursor, .cursorShape = requestedShape});`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 339-341 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 349-351 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Lines 359-361 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 14, cols 8-36 - Cover the true outcome of `p_platformRuntime == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 21, cols 8-25 - Cover the true outcome of `result == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 70, cols 12-23 - Cover the false outcome of `action.type`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 129, cols 11-24 - Cover the false outcome of `p_action.type`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 138, cols 8-42 - Cover the false outcome of `p_action.title.has_value() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 144, cols 8-41 - Cover the false outcome of `p_action.rect.has_value() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 149, cols 3-37 - Cover the true outcome of `case PlatformActionType::SetCursor`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 150, cols 8-48 - Cover the true outcome and false outcome of `p_action.cursorShape.has_value() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 174, cols 7-57 - Cover the false outcome of `_platformResourcesReleased.exchange(true) == false`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 184, cols 9-33 - Cover the true outcome of `_isClosed.load() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 199, cols 7-44 - Cover the true outcome of `requestedShape != _appliedCursorShape`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 226, cols 7-55 - Cover the false outcome of `_renderResourcesReleased.exchange(true) == false`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 234, cols 11-24 - Cover the false outcome of `p_action.type`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 237, cols 8-41 - Cover the false outcome of `p_action.rect.has_value() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 243, cols 8-49 - Cover the false outcome of `p_action.vSyncEnabled.has_value() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 289, cols 53-85 - Cover the false outcome of `_host.isPlatformThread() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 338, cols 7-31 - Cover the true outcome of `_isClosed.load() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 348, cols 7-31 - Cover the true outcome of `_isClosed.load() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 358, cols 7-31 - Cover the true outcome of `_isClosed.load() == true`.
- [ ] `srcs/structures/system/device/window/spk_window.cpp` - Line 450, cols 4-45 - Cover the false outcome of `_platformResourcesReleased.load() == true`.

### srcs/structures/system/device/window/spk_window_handle.cpp

Coverage: lines 86.89% (53/61, missing 8); branches 85.71% (12/14, missing 2); functions 90.00% (9/10, missing 1); regions 85.71% (24/28, missing 4).

Missing functions or function instantiations:

- [ ] `srcs/structures/system/device/window/spk_window_handle.cpp` - Lines 75-84 - Execute the currently uncalled function: `spk::Rect2D WindowHandle::rect() const`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/device/window/spk_window_handle.cpp` - Line 78, cols 7-24 - Cover the true outcome and false outcome of `window == nullptr`.

### srcs/structures/system/device/window/spk_window_host.cpp

Coverage: lines 89.45% (212/237, missing 25); branches 81.11% (73/90, missing 17); functions 96.00% (24/25, missing 1); regions 93.43% (128/137, missing 9).

Missing functions or function instantiations:

- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 154-163 - Execute the currently uncalled function: `void WindowHost::setCursor(const std::string &p_name)`.

Uncovered code regions:

- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 58-62 - Exercise this uncovered code path: `throw std::runtime_error( "spk::WindowHost::" + std::string(p_operation) + " must be called from the render thread");`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 85-87 - Exercise this uncovered code path: `throw std::runtime_error("spk::WindowHost can't create a render context after its frame has been released");`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 273-275 - Exercise this uncovered code path: `throw std::runtime_error("spk::WindowHost::present called without an initialized render context");`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 289-291 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 294-296 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 32, cols 28-61 - Cover the false outcome of `_frame->surfaceState() != nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 57, cols 7-49 - Cover the true outcome of `_renderThreadID.value() != currentThreadID`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 84, cols 7-24 - Cover the true outcome of `_frame == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 90, cols 7-30 - Cover the true outcome of `surfaceState == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 132, cols 47-72 - Cover the true outcome of `_renderContext == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 132, cols 76-110 - Cover the true outcome of `_renderContext->isValid() == false`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 157, cols 7-24 - Cover the true outcome and false outcome of `_frame == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 194, cols 7-24 - Cover the false outcome of `_frame != nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 194, cols 7-61 - Cover the false outcome of `_frame != nullptr && _frame->surfaceState() != nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 194, cols 28-61 - Cover the false outcome of `_frame->surfaceState() != nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 205, cols 47-72 - Cover the false outcome of `_renderContext == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 259, cols 48-73 - Cover the true outcome of `_renderContext == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 259, cols 77-111 - Cover the true outcome of `_renderContext->isValid() == false`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 272, cols 7-32 - Cover the true outcome of `_renderContext == nullptr`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 288, cols 7-44 - Cover the true outcome of `_ensureRenderContextLocked() == false`.
- [ ] `srcs/structures/system/device/window/spk_window_host.cpp` - Line 293, cols 7-41 - Cover the true outcome of `_renderContext->isValid() == false`.

### srcs/structures/system/device/window/spk_window_registry.cpp

Coverage: lines 90.91% (70/77, missing 7); branches 81.25% (13/16, missing 3); functions 100.00% (7/7, missing 0); regions 92.59% (25/27, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/system/device/window/spk_window_registry.cpp` - Lines 19-21 - Exercise this uncovered code path: `throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");`.
- [ ] `srcs/structures/system/device/window/spk_window_registry.cpp` - Lines 38-40 - Exercise this uncovered code path: `throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] doesn't exist");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/device/window/spk_window_registry.cpp` - Line 18, cols 7-38 - Cover the true outcome of `_windows.contains(p_id) == true`.
- [ ] `srcs/structures/system/device/window/spk_window_registry.cpp` - Line 37, cols 7-33 - Cover the true outcome of `iterator == _windows.end()`.
- [ ] `srcs/structures/system/device/window/spk_window_registry.cpp` - Line 79, cols 8-42 - Cover the true outcome of `iterator->second.window == nullptr`.

### srcs/structures/system/time/spk_chronometer.cpp

Coverage: lines 96.34% (79/82, missing 3); branches 96.15% (25/26, missing 1); functions 100.00% (12/12, missing 0); regions 97.14% (34/35, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/system/time/spk_chronometer.cpp` - Lines 73-75 - Exercise this uncovered code path: `return Duration();`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/time/spk_chronometer.cpp` - Line 72, cols 7-31 - Cover the true outcome of `_state != State::Running`.

### srcs/structures/system/win32/spk_winapi_class.cpp

Coverage: lines 95.89% (70/73, missing 3); branches 75.00% (18/24, missing 6); functions 100.00% (6/6, missing 0); regions 97.87% (46/47, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/system/win32/spk_winapi_class.cpp` - Lines 40-42 - Exercise this uncovered code path: `throwLastError("RegisterClassExW");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 39, cols 8-47 - Cover the true outcome of `errorCode != ERROR_CLASS_ALREADY_EXISTS`.
- [ ] `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 62, cols 32-52 - Cover the false outcome of `_instance != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 62, cols 56-84 - Cover the false outcome of `_nativeName.empty() == false`.
- [ ] `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 75, cols 7-28 - Cover the false outcome of `_isRegistered == true`.
- [ ] `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 75, cols 32-52 - Cover the false outcome of `_instance != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 75, cols 56-84 - Cover the false outcome of `_nativeName.empty() == false`.

### srcs/structures/system/win32/spk_winapi_cursor.cpp

Coverage: lines 97.78% (132/135, missing 3); branches 85.00% (17/20, missing 3); functions 100.00% (24/24, missing 0); regions 98.65% (73/74, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Lines 142-144 - Exercise this uncovered code path: `throw std::runtime_error("spk::Cursor::registerCursor: CopyCursor failed for '" + p_name + "'");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Line 63, cols 30-48 - Cover the false outcome of `_handle != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Line 76, cols 30-48 - Cover the false outcome of `_handle != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Line 141, cols 7-22 - Cover the true outcome of `copy == nullptr`.

### srcs/structures/system/win32/spk_winapi_frame.cpp

Coverage: lines 95.43% (209/219, missing 10); branches 90.54% (67/74, missing 7); functions 94.44% (17/18, missing 1); regions 99.07% (106/107, missing 1).

Missing functions or function instantiations:

- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Lines 221-225 - Execute the currently uncalled function: `void Frame::setCursor(const std::string &p_name)`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 20, cols 3-24 - Cover the true outcome of `case WM_RBUTTONDBLCLK`.
- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 24, cols 3-24 - Cover the true outcome of `case WM_MBUTTONDBLCLK`.
- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 136, cols 3-24 - Cover the true outcome of `case WM_RBUTTONDBLCLK`.
- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 137, cols 3-24 - Cover the true outcome of `case WM_MBUTTONDBLCLK`.
- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 154, cols 3-19 - Cover the true outcome of `case WM_SYSKEYUP`.
- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 179, cols 7-38 - Cover the false outcome of `TrackMouseEvent(&event) == TRUE`.
- [ ] `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 234, cols 7-32 - Cover the false outcome of `_window.isValid() == true`.

### srcs/structures/system/win32/spk_winapi_window.cpp

Coverage: lines 90.50% (181/200, missing 19); branches 80.36% (45/56, missing 11); functions 100.00% (18/18, missing 0); regions 94.23% (98/104, missing 6).

Uncovered code regions:

- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 39-41 - Exercise this uncovered code path: `result = DefWindowProcW(_handle, p_message, p_wParam, p_lParam);`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 73-75 - Exercise this uncovered code path: `throwLastError("AdjustWindowRectEx");`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 92-94 - Exercise this uncovered code path: `throwLastError("CreateWindowExW");`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 168-170 - Exercise this uncovered code path: `throwLastError("SetWindowTextW");`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 189-191 - Exercise this uncovered code path: `throwLastError("AdjustWindowRectEx");`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 201-203 - Exercise this uncovered code path: `throwLastError("SetWindowPos");`.

Uncovered branch outcomes:

- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 34, cols 7-28 - Cover the false outcome of `_procedure != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 72, cols 7-80 - Cover the true outcome of `AdjustWindowRectEx(&nativeRect, p_style, FALSE, p_extendedStyle) == FALSE`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 91, cols 7-25 - Cover the true outcome of `_handle == nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 103, cols 7-25 - Cover the false outcome of `_handle != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 127, cols 7-25 - Cover the false outcome of `_handle != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 167, cols 7-92 - Cover the true outcome of `_handle != nullptr && SetWindowTextW(_handle, toWideString(p_title).c_str()) == FALSE`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 167, cols 29-92 - Cover the true outcome of `SetWindowTextW(_handle, toWideString(p_title).c_str()) == FALSE`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 188, cols 7-76 - Cover the true outcome of `AdjustWindowRectEx(&nativeRect, style, FALSE, extendedStyle) == FALSE`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 193-200 - Cover the true outcome of `if (SetWindowPos( _handle, nullptr,`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 208, cols 7-25 - Cover the false outcome of `_handle != nullptr`.
- [ ] `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 272, cols 32-57 - Cover the false outcome of `IsWindow(_handle) == TRUE`.

### srcs/structures/widget/spk_action_bar.cpp

Coverage: lines 92.93% (263/283, missing 20); branches 67.50% (27/40, missing 13); functions 100.00% (29/29, missing 0); regions 89.87% (71/79, missing 8).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Lines 73-75 - Exercise this uncovered code path: `return builder.build();`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Lines 118-120 - Exercise this uncovered code path: `_spriteSheet = std::move(p_spriteSheet); invalidateRenderUnit();`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Lines 164-173 - Exercise this uncovered code path: `_controlHeight = p_controlHeight; for (const auto &element : _elements) if (auto *button = dynamic_cast<spk::PushButton *>(element.item.get()); button != nullptr)`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Lines 166-171 - Exercise this uncovered code path: `if (auto *button = dynamic_cast<spk::PushButton *>(element.item.get()); button != nullptr) applyCompactMetrics(*button, _controlHeight);`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 167 - Exercise this uncovered code path: `button != nullptr`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Lines 168-170 - Exercise this uncovered code path: `applyCompactMetrics(*button, _controlHeight);`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Lines 251-253 - Exercise this uncovered code path: `_onGeometryChange();`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 275 - Exercise this uncovered code path: `_height`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 72, cols 7-30 - Cover the true outcome of `_spriteSheet == nullptr`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 72, cols 34-60 - Cover the true outcome of `geometry().empty() == true`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 86, cols 7-22 - Cover the false outcome of `middleWidth > 0`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 113, cols 7-58 - Cover the false outcome of `p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 1}`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 159, cols 7-40 - Cover the false outcome of `_controlHeight == p_controlHeight`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 165, cols 28-29 - Cover the true outcome and false outcome of `:`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 167, cols 76-93 - Cover the true outcome and false outcome of `button != nullptr`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 196, cols 7-70 - Cover the false outcome of `absoluteGeometry().contains(p_event.device().position) == false`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 246, cols 7-21 - Cover the false outcome of `bar != nullptr`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 273, cols 5-63 - Cover the false outcome of `(_height > static_cast<unsigned int>(BarContentInset * 2))`.
- [ ] `srcs/structures/widget/spk_action_bar.cpp` - Line 333, cols 9-30 - Cover the false outcome of `wasActivated == false`.

### srcs/structures/widget/spk_animation_label.cpp

Coverage: lines 96.04% (97/101, missing 4); branches 76.92% (20/26, missing 6); functions 100.00% (13/13, missing 0); regions 95.83% (46/48, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Lines 71-73 - Exercise this uncovered code path: `_currentSprite = (_currentSprite + 1) % _spriteSheet->sprites().size();`.
- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Line 89 - Exercise this uncovered code path: `0`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Line 31, cols 4-44 - Cover the false outcome of `_spriteSheet->sprites().empty() == false`.
- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Line 32, cols 4-31 - Cover the false outcome of `geometry().empty() == false`.
- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Line 52, cols 34-73 - Cover the true outcome of `_spriteSheet->sprites().empty() == true`.
- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Line 62, cols 7-31 - Cover the false outcome of `_rangeEnd >= _rangeStart`.
- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Line 62, cols 35-77 - Cover the false outcome of `_rangeEnd < _spriteSheet->sprites().size()`.
- [ ] `srcs/structures/widget/spk_animation_label.cpp` - Line 89, cols 15-57 - Cover the false outcome of `(_spriteSheet->sprites().empty() == false)`.

### srcs/structures/widget/spk_checkable_icon_button.cpp

Coverage: lines 89.54% (137/153, missing 16); branches 70.00% (7/10, missing 3); functions 88.46% (23/26, missing 3); regions 90.70% (39/43, missing 4).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 70-77 - Execute the currently uncalled function: `configureMinimalSizeGenerator([this]() {`.
- [ ] `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 169-171 - Execute the currently uncalled function: `const spk::IconButton &CheckableIconButton::uncheckedButton() const`.
- [ ] `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 179-181 - Execute the currently uncalled function: `const spk::IconButton &CheckableIconButton::checkedButton() const`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 123-125 - Exercise this uncovered code path: `_checkedButton.removeIcon();`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_checkable_icon_button.cpp` - Line 117, cols 7-32 - Cover the false outcome of `uncheckedHadIcon == false`.
- [ ] `srcs/structures/widget/spk_checkable_icon_button.cpp` - Line 122, cols 7-30 - Cover the true outcome of `checkedHadIcon == false`.
- [ ] `srcs/structures/widget/spk_checkable_icon_button.cpp` - Line 192, cols 34-53 - Cover the false outcome of `callback != nullptr`.

### srcs/structures/widget/spk_command_panel.cpp

Coverage: lines 94.23% (98/104, missing 6); branches 83.33% (15/18, missing 3); functions 100.00% (13/13, missing 0); regions 94.29% (33/35, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_command_panel.cpp` - Lines 88-90 - Exercise this uncovered code path: `throw std::runtime_error("Button [" + p_name + "] doesn't exist in the command panel [" + name() + "]");`.
- [ ] `srcs/structures/widget/spk_command_panel.cpp` - Lines 98-100 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_command_panel.cpp` - Line 46, cols 58-112 - Cover the true outcome of `_sizePolicy == spk::Layout::SizePolicy::VerticalExtend`.
- [ ] `srcs/structures/widget/spk_command_panel.cpp` - Line 87, cols 7-41 - Cover the true outcome of `_buttons.contains(p_name) == false`.
- [ ] `srcs/structures/widget/spk_command_panel.cpp` - Line 97, cols 7-27 - Cover the true outcome of `it == _buttons.end()`.

### srcs/structures/widget/spk_container_widget.cpp

Coverage: lines 88.00% (44/50, missing 6); branches 70.00% (7/10, missing 3); functions 100.00% (10/10, missing 0); regions 92.00% (23/25, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_container_widget.cpp` - Lines 16-18 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_container_widget.cpp` - Lines 53-55 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_container_widget.cpp` - Line 15, cols 7-26 - Cover the true outcome of `_content == nullptr`.
- [ ] `srcs/structures/widget/spk_container_widget.cpp` - Line 30, cols 7-27 - Cover the false outcome of `p_content != nullptr`.
- [ ] `srcs/structures/widget/spk_container_widget.cpp` - Line 52, cols 7-36 - Cover the true outcome of `_contentSize == p_contentSize`.

### srcs/structures/widget/spk_debug_overlay.cpp

Coverage: lines 100.00% (179/179, missing 0); branches 81.82% (54/66, missing 12); functions 100.00% (19/19, missing 0); regions 98.95% (94/95, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 30 - Exercise this uncovered code path: `_font`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 30, cols 21-39 - Cover the true outcome of `(_font != nullptr)`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 48, cols 9-33 - Cover the false outcome of `row.labels[c] != nullptr`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 62, cols 9-25 - Cover the true outcome of `label == nullptr`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 62, cols 29-53 - Cover the true outcome of `label->font() == nullptr`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 68, cols 24-35 - Cover the true outcome of `area.y == 0`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 76, cols 9-37 - Cover the false outcome of `glyphSize > _outlineSize * 2`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 80, cols 30-55 - Cover the false outcome of `glyphSize > _maxGlyphSize`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 172, cols 9-25 - Cover the false outcome of `label != nullptr`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 191, cols 9-25 - Cover the false outcome of `label != nullptr`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 228, cols 32-70 - Cover the true outcome of `p_column >= _rows[p_row].labels.size()`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 237, cols 32-63 - Cover the true outcome of `_rows[0].labels.empty() == true`.
- [ ] `srcs/structures/widget/spk_debug_overlay.cpp` - Line 237, cols 67-96 - Cover the true outcome of `_rows[0].labels[0] == nullptr`.

### srcs/structures/widget/spk_dynamic_text_label.cpp

Coverage: lines 100.00% (52/52, missing 0); branches 87.50% (7/8, missing 1); functions 100.00% (8/8, missing 0); regions 100.00% (20/20, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_dynamic_text_label.cpp` - Line 58, cols 7-31 - Cover the false outcome of `_textProducer != nullptr`.

### srcs/structures/widget/spk_form_layout.cpp

Coverage: lines 78.45% (233/297, missing 64); branches 58.22% (85/146, missing 61); functions 88.89% (24/27, missing 3); regions 77.78% (140/180, missing 40).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 11-13 - Execute the currently uncalled function: `[[nodiscard]] constexpr uint32_t unlimitedSize()`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 21-23 - Execute the currently uncalled function: `[[nodiscard]] uint32_t clampToUInt(size_t p_value)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 344-346 - Execute the currently uncalled function: `configureFixedSizeGenerator([this]() {`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 17 - Exercise this uncovered code path: `p_element->layout() != nullptr`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 43-45 - Exercise this uncovered code path: `return {0U, 0U};`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 56-58 - Exercise this uncovered code path: `return 0U;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 62-63 - Exercise this uncovered code path: `case spk::FormLayout::SizePolicy::Fixed: return p_element->size().x;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 64-65 - Exercise this uncovered code path: `case spk::FormLayout::SizePolicy::Maximum: return p_element->maximalSize().x;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 72 - Exercise this uncovered code path: `return 0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 82-83 - Exercise this uncovered code path: `case spk::FormLayout::SizePolicy::Fixed: return p_element->size().y;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 84-85 - Exercise this uncovered code path: `case spk::FormLayout::SizePolicy::Maximum: return p_element->maximalSize().y;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 92 - Exercise this uncovered code path: `return 0`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 98-100 - Exercise this uncovered code path: `return false;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 109-111 - Exercise this uncovered code path: `return false;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 130 - Exercise this uncovered code path: `0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 148 - Exercise this uncovered code path: `0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 157 - Exercise this uncovered code path: `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 158-160 - Exercise this uncovered code path: `result.labelColumnExpandable = true;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 193-196 - Exercise this uncovered code path: `p_labelExpandable = true; p_fieldExpandable = true;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 205-207 - Exercise this uncovered code path: `std::fill(p_rowExpandable.begin(), p_rowExpandable.end(), true);`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 227-229 - Exercise this uncovered code path: `p_labelWidth += share + ((remainder-- > 0U) ? 1U : 0U);`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 228 - Exercise this uncovered code path: `(remainder-- > 0U)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 228 - Exercise this uncovered code path: `1U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 228 - Exercise this uncovered code path: `0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 232 - Exercise this uncovered code path: `1U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 245-247 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 281-283 - Exercise this uncovered code path: `labelAvailableWidth = p_availableSize.x;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 285-287 - Exercise this uncovered code path: `fieldAvailableWidth = p_availableSize.x;`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 290 - Exercise this uncovered code path: `0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 292 - Exercise this uncovered code path: `0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 323-330 - Exercise this uncovered code path: `else if (labelRenderable == true) label->setGeometry(spk::Rect2D({p_geometry.anchor.x, y}, {rowWidth, height})); else if (fieldRenderable == true)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 323 - Exercise this uncovered code path: `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 324-326 - Exercise this uncovered code path: `label->setGeometry(spk::Rect2D({p_geometry.anchor.x, y}, {rowWidth, height}));`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 327-330 - Exercise this uncovered code path: `else if (fieldRenderable == true) field->setGeometry(spk::Rect2D({p_geometry.anchor.x, y}, {rowWidth, height}));`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 327 - Exercise this uncovered code path: `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 328-330 - Exercise this uncovered code path: `field->setGeometry(spk::Rect2D({p_geometry.anchor.x, y}, {rowWidth, height}));`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 384 - Exercise this uncovered code path: `0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 392 - Exercise this uncovered code path: `0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Lines 414-416 - Exercise this uncovered code path: `return {0U, 0U};`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 433 - Exercise this uncovered code path: `0U`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 17, cols 10-32 - Cover the false outcome of `(p_element != nullptr)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 17, cols 37-67 - Cover the false outcome of `p_element->widget() != nullptr`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 17, cols 71-101 - Cover the true outcome and false outcome of `p_element->layout() != nullptr`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 42, cols 7-40 - Cover the true outcome of `hasRenderable(p_element) == false`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 55, cols 7-40 - Cover the true outcome of `hasRenderable(p_element) == false`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 60, cols 11-34 - Cover the false outcome of `p_element->sizePolicy()`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 62, cols 3-42 - Cover the true outcome of `case spk::FormLayout::SizePolicy::Fixed`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 64, cols 3-44 - Cover the true outcome of `case spk::FormLayout::SizePolicy::Maximum`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 67, cols 3-53 - Cover the true outcome of `case spk::FormLayout::SizePolicy::HorizontalExtend`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 69, cols 3-51 - Cover the true outcome of `case spk::FormLayout::SizePolicy::VerticalExtend`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 80, cols 11-34 - Cover the false outcome of `p_element->sizePolicy()`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 82, cols 3-42 - Cover the true outcome of `case spk::FormLayout::SizePolicy::Fixed`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 84, cols 3-44 - Cover the true outcome of `case spk::FormLayout::SizePolicy::Maximum`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 87, cols 3-53 - Cover the true outcome of `case spk::FormLayout::SizePolicy::HorizontalExtend`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 89, cols 3-51 - Cover the true outcome of `case spk::FormLayout::SizePolicy::VerticalExtend`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 97, cols 7-27 - Cover the true outcome of `p_element == nullptr`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 103, cols 60-115 - Cover the true outcome of `policy == spk::FormLayout::SizePolicy::HorizontalExtend`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 108, cols 7-27 - Cover the true outcome of `p_element == nullptr`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 113, cols 60-113 - Cover the true outcome of `policy == spk::FormLayout::SizePolicy::VerticalExtend`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 130, cols 11-40 - Cover the false outcome of `p_scan.hasLabelColumn == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 130, cols 44-73 - Cover the false outcome of `p_scan.hasFieldColumn == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 148, cols 31-54 - Cover the false outcome of `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 148, cols 58-81 - Cover the false outcome of `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 150, cols 8-31 - Cover the false outcome of `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 157, cols 9-53 - Cover the true outcome of `elementHorizontallyExpandable(label) == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 157, cols 57-80 - Cover the true outcome and false outcome of `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 163, cols 8-31 - Cover the false outcome of `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 168, cols 9-32 - Cover the false outcome of `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 178, cols 9-53 - Cover the false outcome of `elementHorizontallyExpandable(field) == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 178, cols 57-80 - Cover the false outcome of `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 184, cols 33-67 - Cover the true outcome of `elementVerticallyExpandable(label)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 184, cols 71-105 - Cover the false outcome of `elementVerticallyExpandable(field)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 192, cols 7-33 - Cover the false outcome of `p_labelExpandable == false`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 192, cols 37-63 - Cover the true outcome of `p_fieldExpandable == false`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 204, cols 7-19 - Cover the true outcome of `any == false`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 218, cols 7-24 - Cover the true outcome of `expandables == 0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 226, cols 7-32 - Cover the true outcome of `p_labelExpandable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 228, cols 29-47 - Cover the true outcome and false outcome of `(remainder-- > 0U)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 230, cols 7-32 - Cover the false outcome of `p_fieldExpandable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 232, cols 29-47 - Cover the true outcome of `(remainder-- > 0U)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 244, cols 7-18 - Cover the true outcome of `count == 0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 254, cols 8-34 - Cover the false outcome of `p_rowExpandable[i] == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 280, cols 8-31 - Cover the false outcome of `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 280, cols 35-59 - Cover the true outcome of `fieldRenderable == false`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 284, cols 13-36 - Cover the false outcome of `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 284, cols 40-64 - Cover the true outcome of `labelRenderable == false`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 290, cols 5-20 - Cover the false outcome of `labelRenderable`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 292, cols 5-20 - Cover the false outcome of `fieldRenderable`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 318, cols 8-31 - Cover the false outcome of `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 318, cols 35-58 - Cover the false outcome of `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 323, cols 13-36 - Cover the true outcome and false outcome of `labelRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 327, cols 13-36 - Cover the true outcome and false outcome of `fieldRenderable == true`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 384, cols 31-66 - Cover the false outcome of `(p_geometry.size.x > minTotalWidth)`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 392, cols 48-61 - Cover the false outcome of `rowCount > 0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 413, cols 7-26 - Cover the true outcome of `rowCountValue == 0U`.
- [ ] `srcs/structures/widget/spk_form_layout.cpp` - Line 433, cols 27-45 - Cover the false outcome of `rowCountValue > 0U`.

### srcs/structures/widget/spk_game_engine_widget.cpp

Coverage: lines 100.00% (53/53, missing 0); branches 75.00% (3/4, missing 1); functions 100.00% (25/25, missing 0); regions 100.00% (34/34, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_game_engine_widget.cpp` - Line 28, cols 33-55 - Cover the true outcome of `frameBufferSize.y == 0`.

### srcs/structures/widget/spk_icon_button.cpp

Coverage: lines 87.76% (43/49, missing 6); branches 50.00% (3/6, missing 3); functions 100.00% (9/9, missing 0); regions 90.48% (19/21, missing 2).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_icon_button.cpp` - Lines 29-31 - Exercise this uncovered code path: `throw std::invalid_argument("IconButton iconset cannot be null");`.
- [ ] `srcs/structures/widget/spk_icon_button.cpp` - Lines 59-61 - Exercise this uncovered code path: `_iconset = std::move(p_iconset); _refreshIcon();`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_icon_button.cpp` - Line 28, cols 7-26 - Cover the true outcome of `_iconset == nullptr`.
- [ ] `srcs/structures/widget/spk_icon_button.cpp` - Line 45, cols 7-43 - Cover the false outcome of `p_style.iconSpriteSheet() != nullptr`.
- [ ] `srcs/structures/widget/spk_icon_button.cpp` - Line 54, cols 7-27 - Cover the false outcome of `p_iconset == nullptr`.

### srcs/structures/widget/spk_image_label.cpp

Coverage: lines 94.44% (51/54, missing 3); branches 80.00% (8/10, missing 2); functions 100.00% (9/9, missing 0); regions 95.83% (23/24, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_image_label.cpp` - Lines 67-69 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_image_label.cpp` - Line 30, cols 30-57 - Cover the false outcome of `geometry().empty() == false`.
- [ ] `srcs/structures/widget/spk_image_label.cpp` - Line 66, cols 7-24 - Cover the true outcome of `_depth == p_depth`.

### srcs/structures/widget/spk_interface_window.cpp

Coverage: lines 89.66% (364/406, missing 42); branches 66.28% (57/86, missing 29); functions 92.59% (50/54, missing 4); regions 85.90% (134/156, missing 22).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 155-157 - Execute the currently uncalled function: `const spk::TextLabel &IInterfaceWindow::MenuBar::titleLabel() const`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 165-167 - Execute the currently uncalled function: `const spk::PushButton &IInterfaceWindow::MenuBar::minimizeButton() const`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 175-177 - Execute the currently uncalled function: `const spk::PushButton &IInterfaceWindow::MenuBar::maximizeButton() const`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 185-187 - Execute the currently uncalled function: `const spk::PushButton &IInterfaceWindow::MenuBar::closeButton() const`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 27 - Exercise this uncovered code path: `std::numeric_limits<uint32_t>::max()`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 82 - Exercise this uncovered code path: `height`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 92 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 93 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 122-124 - Exercise this uncovered code path: `case Button::Maximize: _maximizeButton.activate(); break;`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 125-127 - Exercise this uncovered code path: `case Button::Close: _closeButton.activate(); break;`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 139-141 - Exercise this uncovered code path: `case Button::Maximize: _maximizeButton.deactivate(); break;`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 142-144 - Exercise this uncovered code path: `case Button::Close: _closeButton.deactivate(); break;`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 232 - Exercise this uncovered code path: `throw std::invalid_argument("Unknown interface window event")`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 242 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 303-305 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 318-322 - Exercise this uncovered code path: `if (absoluteGeometry().contains(p_event.device().position) == true) p_event.consume();`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 318 - Exercise this uncovered code path: `absoluteGeometry().contains(p_event.device().position) == true`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 319-321 - Exercise this uncovered code path: `p_event.consume();`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 329-331 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 403 - Exercise this uncovered code path: `*_contentPadding == p_padding`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 404-406 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Lines 421-423 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 26, cols 10-67 - Cover the true outcome of `(p_left > std::numeric_limits<uint32_t>::max() - p_right)`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 82, cols 4-52 - Cover the false outcome of `(height > static_cast<unsigned int>(Margin * 2))`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 92, cols 5-34 - Cover the false outcome of `_maximizeButton.isActivated()`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 93, cols 5-31 - Cover the false outcome of `_closeButton.isActivated()`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 117, cols 11-19 - Cover the false outcome of `p_button`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 119, cols 3-24 - Cover the false outcome of `case Button::Minimize`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 122, cols 3-24 - Cover the true outcome of `case Button::Maximize`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 125, cols 3-21 - Cover the true outcome of `case Button::Close`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 134, cols 11-19 - Cover the false outcome of `p_button`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 136, cols 3-24 - Cover the false outcome of `case Button::Minimize`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 139, cols 3-24 - Cover the true outcome of `case Button::Maximize`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 142, cols 3-21 - Cover the true outcome of `case Button::Close`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 222, cols 11-18 - Cover the false outcome of `p_event`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 242, cols 4-43 - Cover the false outcome of `(geometry().width() > horizontalMargin)`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 289, cols 7-35 - Cover the true outcome of `p_event.isConsumed() == true`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 302, cols 7-35 - Cover the true outcome of `p_event.isConsumed() == true`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 307, cols 7-42 - Cover the false outcome of `p_event->button == spk::Mouse::Left`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 308, cols 4-91 - Cover the false outcome of `_menuBar.titleLabel().viewport().geometry().contains(p_event.device().position) == true`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 318, cols 7-69 - Cover the true outcome and false outcome of `absoluteGeometry().contains(p_event.device().position) == true`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 328, cols 7-42 - Cover the true outcome of `p_event->button != spk::Mouse::Left`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 328, cols 46-64 - Cover the true outcome of `_isMoving == false`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 403, cols 7-42 - Cover the true outcome of `_contentPadding.has_value() == true`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 403, cols 46-75 - Cover the true outcome and false outcome of `*_contentPadding == p_padding`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 410, cols 7-26 - Cover the false outcome of `_content != nullptr`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 420, cols 7-43 - Cover the true outcome of `_contentPadding.has_value() == false`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 427, cols 7-26 - Cover the false outcome of `_content != nullptr`.
- [ ] `srcs/structures/widget/spk_interface_window.cpp` - Line 454, cols 7-26 - Cover the false outcome of `_content != nullptr`.

### srcs/structures/widget/spk_layout.cpp

Coverage: lines 83.70% (154/184, missing 30); branches 63.04% (29/46, missing 17); functions 93.10% (27/29, missing 2); regions 82.35% (84/102, missing 18).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 137-140 - Execute the currently uncalled function: `spk::Vector2UInt Layout::fixedSize() const`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 143-146 - Execute the currently uncalled function: `spk::Vector2UInt Layout::maximalSize() const`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 56 - Exercise this uncovered code path: `return {0u, 0u}`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 65-70 - Exercise this uncovered code path: `if (_layout != nullptr) return _layout->preferredSizeFor(p_availableSize); return {0u, 0u};`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 65 - Exercise this uncovered code path: `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 66-68 - Exercise this uncovered code path: `return _layout->preferredSizeFor(p_availableSize);`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 69 - Exercise this uncovered code path: `return {0u, 0u}`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 78-83 - Exercise this uncovered code path: `if (_layout != nullptr) return _layout->fixedSize(); return {0u, 0u};`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 78 - Exercise this uncovered code path: `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 79-81 - Exercise this uncovered code path: `return _layout->fixedSize();`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 82 - Exercise this uncovered code path: `return {0u, 0u}`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 91-96 - Exercise this uncovered code path: `if (_layout != nullptr) return _layout->maximalSize(); return {0u, 0u};`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 91 - Exercise this uncovered code path: `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 92-94 - Exercise this uncovered code path: `return _layout->maximalSize();`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 95 - Exercise this uncovered code path: `return {0u, 0u}`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 202 - Exercise this uncovered code path: `++it`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Lines 218-220 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 222 - Exercise this uncovered code path: `++it`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 52, cols 7-25 - Cover the false outcome of `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 61, cols 7-25 - Cover the false outcome of `_widget != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 65, cols 7-25 - Cover the true outcome and false outcome of `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 74, cols 7-25 - Cover the false outcome of `_widget != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 78, cols 7-25 - Cover the true outcome and false outcome of `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 87, cols 7-25 - Cover the false outcome of `_widget != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 91, cols 7-25 - Cover the true outcome and false outcome of `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 104, cols 12-30 - Cover the false outcome of `_layout != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 189, cols 12-41 - Cover the false outcome of `p_element->isLayout() == true`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 204, cols 8-22 - Cover the false outcome of `*it != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 204, cols 26-53 - Cover the false outcome of `(*it)->widget() == p_widget`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 217, cols 7-26 - Cover the true outcome of `p_layout == nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 224, cols 8-22 - Cover the false outcome of `*it != nullptr`.
- [ ] `srcs/structures/widget/spk_layout.cpp` - Line 224, cols 26-53 - Cover the false outcome of `(*it)->layout() == p_layout`.

### srcs/structures/widget/spk_message_box.cpp

Coverage: lines 92.56% (199/215, missing 16); branches 62.50% (5/8, missing 3); functions 87.18% (34/39, missing 5); regions 87.27% (48/55, missing 7).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_message_box.cpp` - Lines 53-55 - Execute the currently uncalled function: `const spk::VerticalLayout &MessageBox::Content::layout() const`.
- [ ] `srcs/structures/widget/spk_message_box.cpp` - Lines 83-85 - Execute the currently uncalled function: `_closeContract = subscribeTo(spk::IInterfaceWindow::Event::Close, [this]() {`.
- [ ] `srcs/structures/widget/spk_message_box.cpp` - Lines 199-200 - Execute the currently uncalled function: `configure("Yes", []() {`.
- [ ] `srcs/structures/widget/spk_message_box.cpp` - Lines 202-203 - Execute the currently uncalled function: `[]() {`.
- [ ] `srcs/structures/widget/spk_message_box.cpp` - Lines 234-239 - Execute the currently uncalled function: `[action = p_secondAction]() {`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_message_box.cpp` - Line 225, cols 9-26 - Cover the false outcome of `action != nullptr`.
- [ ] `srcs/structures/widget/spk_message_box.cpp` - Line 235, cols 9-26 - Cover the true outcome and false outcome of `action != nullptr`.

### srcs/structures/widget/spk_panel.cpp

Coverage: lines 96.97% (96/99, missing 3); branches 81.25% (13/16, missing 3); functions 100.00% (17/17, missing 0); regions 97.56% (40/41, missing 1).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_panel.cpp` - Lines 122-124 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_panel.cpp` - Line 85, cols 7-30 - Cover the false outcome of `_spriteSheet != nullptr`.
- [ ] `srcs/structures/widget/spk_panel.cpp` - Line 85, cols 34-61 - Cover the false outcome of `geometry().empty() == false`.
- [ ] `srcs/structures/widget/spk_panel.cpp` - Line 121, cols 7-34 - Cover the true outcome of `_cornerSize == p_cornerSize`.

### srcs/structures/widget/spk_push_button.cpp

Coverage: lines 92.04% (312/339, missing 27); branches 79.17% (38/48, missing 10); functions 95.65% (44/46, missing 2); regions 89.66% (104/116, missing 12).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_push_button.cpp` - Lines 266-275 - Execute the currently uncalled function: `void PushButton::resetIconSize()`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Lines 290-299 - Execute the currently uncalled function: `void PushButton::resetIconPadding()`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_push_button.cpp` - Lines 21-23 - Exercise this uncovered code path: `return {0, 0};`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Lines 256-258 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 279 - Exercise this uncovered code path: `*_iconPadding == p_iconPadding`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Lines 280-282 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 20, cols 8-35 - Cover the true outcome of `p_icon.texture() == nullptr`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 255, cols 7-30 - Cover the true outcome of `_iconSize == p_iconSize`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 267, cols 7-37 - Cover the true outcome and false outcome of `_iconSize.has_value() == false`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 279, cols 7-39 - Cover the true outcome of `_iconPadding.has_value() == true`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 279, cols 43-73 - Cover the true outcome and false outcome of `*_iconPadding == p_iconPadding`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 291, cols 7-40 - Cover the true outcome and false outcome of `_iconPadding.has_value() == false`.
- [ ] `srcs/structures/widget/spk_push_button.cpp` - Line 439, cols 7-42 - Cover the true outcome of `p_event->button != spk::Mouse::Left`.

### srcs/structures/widget/spk_resizable_element.cpp

Coverage: lines 90.57% (48/53, missing 5); branches - (0/0, missing 0); functions 94.44% (17/18, missing 1); regions 94.44% (17/18, missing 1).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_resizable_element.cpp` - Lines 11-15 - Execute the currently uncalled function: `void ResizableElement::configureSizeGenerators( SizeGenerator p_minimalGenerator, SizeGenerator p_fixedGenerator, SizeGenerator p_maximalGenerator)`.

### srcs/structures/widget/spk_scalable_widget.cpp

Coverage: lines 90.29% (186/206, missing 20); branches 71.43% (40/56, missing 16); functions 100.00% (14/14, missing 0); regions 92.59% (75/81, missing 6).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 95-97 - Exercise this uncovered code path: `edges |= Edge::Bottom;`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 116 - Exercise this uncovered code path: `right`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 120 - Exercise this uncovered code path: `left`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 125-127 - Exercise this uncovered code path: `p_mouse.requestCursor("ResizeNS");`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 174-184 - Exercise this uncovered code path: `compute1DResize( _baseGeometry.anchor.y, static_cast<int>(_baseGeometry.size.y),`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 242-244 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 94, cols 7-51 - Cover the true outcome of `bottomArea.contains(p_mousePosition) == true`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 116, cols 25-31 - Cover the true outcome of `bottom`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 116, cols 35-40 - Cover the true outcome and false outcome of `right`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 120, cols 20-25 - Cover the false outcome of `right`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 120, cols 31-37 - Cover the true outcome of `bottom`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 120, cols 41-45 - Cover the true outcome and false outcome of `left`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 124, cols 12-15 - Cover the true outcome of `top`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 124, cols 19-25 - Cover the true outcome of `bottom`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 128, cols 12-16 - Cover the true outcome of `left`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 173, cols 12-46 - Cover the true outcome of `(_activeEdges & Edge::Bottom) != 0`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 198, cols 12-45 - Cover the false outcome of `(_activeEdges & Edge::Right) != 0`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 232, cols 7-32 - Cover the false outcome of `newGeometry != geometry()`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 241, cols 7-42 - Cover the true outcome of `p_event->button != spk::Mouse::Left`.
- [ ] `srcs/structures/widget/spk_scalable_widget.cpp` - Line 257, cols 7-42 - Cover the true outcome of `p_event->button != spk::Mouse::Left`.

### srcs/structures/widget/spk_screen.cpp

Coverage: lines 100.00% (26/26, missing 0); branches 83.33% (5/6, missing 1); functions 100.00% (4/4, missing 0); regions 100.00% (11/11, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_screen.cpp` - Line 9, cols 36-57 - Cover the false outcome of `_activeScreen != this`.

### srcs/structures/widget/spk_scroll_area.cpp

Coverage: lines 97.74% (130/133, missing 3); branches 82.35% (28/34, missing 6); functions 100.00% (22/22, missing 0); regions 95.83% (69/72, missing 3).

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 31 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 36 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Lines 119-121 - Exercise this uncovered code path: `bar.activate();`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 31, cols 12-37 - Cover the false outcome of `(width > _scrollBarWidth)`.
- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 36, cols 13-39 - Cover the false outcome of `(height > _scrollBarWidth)`.
- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 61, cols 27-40 - Cover the false outcome of `content.x > 0`.
- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 65, cols 27-40 - Cover the false outcome of `content.y > 0`.
- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 94, cols 7-47 - Cover the false outcome of `_verticalScrollBar.isActivated() == true`.
- [ ] `srcs/structures/widget/spk_scroll_area.cpp` - Line 118, cols 7-22 - Cover the true outcome of `p_state == true`.

### srcs/structures/widget/spk_scroll_bar.cpp

Coverage: lines 88.98% (105/118, missing 13); branches 80.00% (16/20, missing 4); functions 86.36% (19/22, missing 3); regions 88.10% (37/42, missing 5).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 151-153 - Execute the currently uncalled function: `const spk::PushButton &ScrollBar::negativeButton() const`.
- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 161-163 - Execute the currently uncalled function: `const spk::PushButton &ScrollBar::positiveButton() const`.
- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 171-173 - Execute the currently uncalled function: `const spk::SliderBar &ScrollBar::sliderBar() const`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Line 77 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 127-128 - Exercise this uncovered code path: `_step = p_step;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Line 56, cols 11-35 - Cover the false outcome of `_sliderBar.orientation()`.
- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Line 71, cols 11-35 - Cover the false outcome of `_sliderBar.orientation()`.
- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Line 77, cols 5-42 - Cover the false outcome of `(geometry().width() > buttonSize * 2)`.
- [ ] `srcs/structures/widget/spk_scroll_bar.cpp` - Line 122, cols 25-38 - Cover the false outcome of `p_step > 1.0f`.

### srcs/structures/widget/spk_slider_bar.cpp

Coverage: lines 83.49% (182/218, missing 36); branches 68.75% (33/48, missing 15); functions 85.71% (24/28, missing 4); regions 82.47% (80/97, missing 17).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 49-51 - Execute the currently uncalled function: `void SliderBar::applyStyle(const spk::WidgetStyle &p_style)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 295-297 - Execute the currently uncalled function: `spk::Panel &SliderBar::background()`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 300-302 - Execute the currently uncalled function: `const spk::Panel &SliderBar::background() const`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 310-312 - Execute the currently uncalled function: `const spk::Panel &SliderBar::body() const`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 109 - Exercise this uncovered code path: `effectiveBodyLength(geometry().height(), geometry().width(), _scale)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 113 - Exercise this uncovered code path: `static_cast<float>(geometry().height() - bodyLength)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 116-118 - Exercise this uncovered code path: `return _ratio;`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 123 - Exercise this uncovered code path: `static_cast<float>(p_position.y - absoluteGeometry().top())`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 131-133 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 155-157 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 174 - Exercise this uncovered code path: `effectiveBodyLength(geometry().height(), geometry().width(), _scale)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 178 - Exercise this uncovered code path: `static_cast<float>(geometry().height() - bodyLength)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 181-183 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 188 - Exercise this uncovered code path: `static_cast<float>(p_event.device().position.y - _draggedMousePosition.y)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 192-194 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 205-207 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Lines 251-254 - Exercise this uncovered code path: `setRatio(0.0f); return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 63, cols 11-23 - Cover the false outcome of `_orientation`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 107, cols 4-14 - Cover the false outcome of `horizontal`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 111, cols 4-14 - Cover the false outcome of `horizontal`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 115, cols 7-20 - Cover the true outcome of `range <= 0.0f`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 121, cols 4-14 - Cover the false outcome of `horizontal`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 130, cols 7-42 - Cover the true outcome of `p_event->button != spk::Mouse::Left`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 154, cols 7-42 - Cover the true outcome of `p_event->button != spk::Mouse::Left`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 154, cols 46-65 - Cover the true outcome of `_isDragged == false`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 172, cols 4-50 - Cover the false outcome of `(_orientation == spk::Orientation::Horizontal)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 176, cols 4-50 - Cover the false outcome of `(_orientation == spk::Orientation::Horizontal)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 180, cols 7-20 - Cover the true outcome of `range <= 0.0f`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 186, cols 4-50 - Cover the false outcome of `(_orientation == spk::Orientation::Horizontal)`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 191, cols 7-25 - Cover the true outcome of `newRatio == _ratio`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 204, cols 7-36 - Cover the true outcome of `_orientation == p_orientation`.
- [ ] `srcs/structures/widget/spk_slider_bar.cpp` - Line 250, cols 7-29 - Cover the true outcome of `_maxValue == _minValue`.

### srcs/structures/widget/spk_text_area.cpp

Coverage: lines 79.41% (243/306, missing 63); branches 66.25% (53/80, missing 27); functions 86.49% (32/37, missing 5); regions 81.75% (112/137, missing 25).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 325-333 - Execute the currently uncalled function: `void TextArea::setDepth(float p_depth)`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 404-406 - Execute the currently uncalled function: `const std::shared_ptr<spk::Font> &TextArea::font() const`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 414-416 - Execute the currently uncalled function: `const spk::Color &TextArea::glyphColor() const`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 419-421 - Execute the currently uncalled function: `const spk::Color &TextArea::outlineColor() const`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 424-426 - Execute the currently uncalled function: `float TextArea::depth() const`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 111-113 - Exercise this uncovered code path: `return 0;`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 130-132 - Exercise this uncovered code path: `return result;`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 137-140 - Exercise this uncovered code path: `result.push_back(paragraph); continue;`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 185-187 - Exercise this uncovered code path: `return builder.build();`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 198 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 204-206 - Exercise this uncovered code path: `lineTop = geometry().top() + (static_cast<int>(geometry().height()) - static_cast<int>(blockHeight)) / 2;`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 208-210 - Exercise this uncovered code path: `lineTop = geometry().bottom() - static_cast<int>(blockHeight);`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 218-220 - Exercise this uncovered code path: `anchorX = geometry().left() + static_cast<int>(geometry().width() / 2);`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 222-224 - Exercise this uncovered code path: `anchorX = geometry().right();`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 247-249 - Exercise this uncovered code path: `return {0, 0};`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 268 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 294-296 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 309-311 - Exercise this uncovered code path: `_glyphColor = p_color; invalidateRenderUnit();`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 338-340 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 361-363 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 373 - Exercise this uncovered code path: `_verticalAlignment == p_verticalAlignment`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Lines 374-376 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 15, cols 7-28 - Cover the false outcome of `p_left.g == p_right.g`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 16, cols 7-28 - Cover the false outcome of `p_left.b == p_right.b`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 17, cols 7-28 - Cover the false outcome of `p_left.a == p_right.a`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 25, cols 10-32 - Cover the false outcome of `start <= p_text.size()`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 110, cols 7-23 - Cover the true outcome of `_font == nullptr`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 129, cols 7-23 - Cover the true outcome of `_font == nullptr`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 136, cols 8-33 - Cover the true outcome of `paragraph.empty() == true`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 171, cols 8-36 - Cover the false outcome of `currentLine.empty() == false`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 184, cols 7-33 - Cover the true outcome of `geometry().empty() == true`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 189, cols 7-23 - Cover the true outcome of `_font == nullptr`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 197, cols 4-27 - Cover the true outcome of `(lines.empty() == true)`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 203, cols 7-61 - Cover the true outcome of `_verticalAlignment == spk::VerticalAlignment::Centered`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 207, cols 12-62 - Cover the true outcome of `_verticalAlignment == spk::VerticalAlignment::Down`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 214, cols 8-29 - Cover the false outcome of `line.empty() == false`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 217, cols 9-67 - Cover the true outcome of `_horizontalAlignment == spk::HorizontalAlignment::Centered`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 221, cols 14-69 - Cover the true outcome of `_horizontalAlignment == spk::HorizontalAlignment::Right`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 246, cols 7-23 - Cover the true outcome of `_font == nullptr`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 267, cols 4-27 - Cover the true outcome of `(lines.empty() == true)`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 293, cols 7-30 - Cover the true outcome of `_textSize == p_textSize`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 304, cols 7-46 - Cover the false outcome of `sameColor(_glyphColor, p_color) == true`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 326, cols 7-24 - Cover the true outcome and false outcome of `_depth == p_depth`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 337, cols 7-22 - Cover the true outcome of `_text == p_text`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 360, cols 7-36 - Cover the true outcome of `_linePadding == p_linePadding`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 373, cols 7-52 - Cover the true outcome of `_horizontalAlignment == p_horizontalAlignment`.
- [ ] `srcs/structures/widget/spk_text_area.cpp` - Line 373, cols 56-97 - Cover the true outcome and false outcome of `_verticalAlignment == p_verticalAlignment`.

### srcs/structures/widget/spk_text_edit.cpp

Coverage: lines 76.88% (409/532, missing 123); branches 63.82% (97/152, missing 55); functions 81.67% (49/60, missing 11); regions 77.64% (184/237, missing 53).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 60-71 - Execute the currently uncalled function: `configureMinimalSizeGenerator([this]() {`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 239-250 - Execute the currently uncalled function: `void TextEdit::_onUpdate(const spk::UpdateTick &p_tick)`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 402-404 - Execute the currently uncalled function: `TextEdit::ValidationState TextEdit::validationState() const`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 544-552 - Execute the currently uncalled function: `void TextEdit::setCursorColor(const spk::Color &p_color)`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 555-563 - Execute the currently uncalled function: `void TextEdit::setDepth(float p_depth)`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 670-672 - Execute the currently uncalled function: `const std::shared_ptr<spk::SpriteSheet> &TextEdit::spriteSheet() const`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 675-677 - Execute the currently uncalled function: `const spk::Vector2Int &TextEdit::cornerSize() const`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 680-682 - Execute the currently uncalled function: `const std::shared_ptr<spk::Font> &TextEdit::font() const`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 690-692 - Execute the currently uncalled function: `const spk::Color &TextEdit::glyphColor() const`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 695-697 - Execute the currently uncalled function: `const spk::Color &TextEdit::outlineColor() const`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 700-702 - Execute the currently uncalled function: `float TextEdit::depth() const`.

Uncovered code regions:

- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 32-49 - Exercise this uncovered code path: `else if (codepoint <= 0x7FF) result.push_back(static_cast<char>(0xC0 | (codepoint >> 6))); result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 32 - Exercise this uncovered code path: `codepoint <= 0x7FF`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 33-36 - Exercise this uncovered code path: `result.push_back(static_cast<char>(0xC0 | (codepoint >> 6))); result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 37-49 - Exercise this uncovered code path: `else if (codepoint <= 0xFFFF) result.push_back(static_cast<char>(0xE0 | (codepoint >> 12))); result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 37 - Exercise this uncovered code path: `codepoint <= 0xFFFF`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 38-42 - Exercise this uncovered code path: `result.push_back(static_cast<char>(0xE0 | (codepoint >> 12))); result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F))); result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 44-49 - Exercise this uncovered code path: `result.push_back(static_cast<char>(0xF0 | (codepoint >> 18))); result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F))); result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 130-132 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 173-175 - Exercise this uncovered code path: `return builder.build();`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 215 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 279-281 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 290-293 - Exercise this uncovered code path: `releaseFocus(FocusType::Keyboard); invalidateRenderUnit();`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 299-301 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 321-322 - Exercise this uncovered code path: `default: return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 347-349 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 353-356 - Exercise this uncovered code path: `_lowerCursor = _cursor; _needHigherCursorUpdate = true;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 363-365 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 409-411 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 422-424 - Exercise this uncovered code path: `return;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 430-433 - Exercise this uncovered code path: `_lowerCursor = (_lowerCursor > 0 && _cursor > 0) ? _cursor - 1 : 0; _needHigherCursorUpdate = true;`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 431 - Exercise this uncovered code path: `(_lowerCursor > 0 && _cursor > 0)`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 431 - Exercise this uncovered code path: `_lowerCursor > 0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 431 - Exercise this uncovered code path: `_cursor > 0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 431 - Exercise this uncovered code path: `_cursor - 1`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 431 - Exercise this uncovered code path: `0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 469-471 - Exercise this uncovered code path: `throw std::invalid_argument("TextEdit sprite sheet must contain a 3x3 grid");`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 528-530 - Exercise this uncovered code path: `_glyphColor = p_color; invalidateRenderUnit();`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 599-601 - Exercise this uncovered code path: `return;`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 16, cols 7-28 - Cover the false outcome of `p_left.g == p_right.g`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 17, cols 7-28 - Cover the false outcome of `p_left.b == p_right.b`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 18, cols 7-28 - Cover the false outcome of `p_left.a == p_right.a`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 28, cols 8-25 - Cover the false outcome of `codepoint <= 0x7F`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 32, cols 13-31 - Cover the true outcome and false outcome of `codepoint <= 0x7FF`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 37, cols 13-32 - Cover the true outcome and false outcome of `codepoint <= 0xFFFF`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 63, cols 8-24 - Cover the true outcome and false outcome of `_font != nullptr`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 63, cols 28-57 - Cover the true outcome and false outcome of `_placeholder.empty() == false`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 129, cols 7-23 - Cover the true outcome of `_font == nullptr`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 137, cols 7-37 - Cover the false outcome of `_needLowerCursorUpdate == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 142-144 - Cover the false outcome of `_font->computeStringSize( textToRender.substr(_lowerCursor - 1, _cursor - _lowerCursor + 1), _textSize) .x <= availableWidth)`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 152, cols 7-38 - Cover the false outcome of `_needHigherCursorUpdate == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Lines 157-159 - Cover the false outcome of `_font->computeStringSize( textToRender.substr(_lowerCursor, _higherCursor - _lowerCursor + 1), _textSize) .x <= availableWidth)`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 172, cols 7-33 - Cover the true outcome of `geometry().empty() == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 177, cols 7-30 - Cover the false outcome of `_spriteSheet != nullptr`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 192, cols 7-23 - Cover the false outcome of `_font != nullptr`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 192, cols 27-55 - Cover the false outcome of `visibleText.empty() == false`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 210, cols 7-23 - Cover the false outcome of `_font != nullptr`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 211, cols 4-26 - Cover the false outcome of `_isEditEnabled == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 212, cols 4-25 - Cover the false outcome of `_renderCursor == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 215, cols 33-58 - Cover the false outcome of `(_cursor >= _lowerCursor)`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 242, cols 7-39 - Cover the true outcome and false outcome of `newRenderCursor != _renderCursor`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 245, cols 8-45 - Cover the true outcome and false outcome of `hasFocus(FocusType::Keyboard) == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 278, cols 7-42 - Cover the true outcome of `p_event->button != spk::Mouse::Left`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 289, cols 12-49 - Cover the true outcome of `hasFocus(FocusType::Keyboard) == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 298, cols 7-30 - Cover the true outcome of `_isEditEnabled == false`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 298, cols 34-72 - Cover the true outcome of `hasFocus(FocusType::Keyboard) == false`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 321, cols 3-10 - Cover the true outcome of `default`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 346, cols 7-19 - Cover the true outcome of `_cursor == 0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 352, cols 7-29 - Cover the true outcome of `_cursor < _lowerCursor`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 362, cols 7-30 - Cover the true outcome of `_cursor >= _text.size()`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 368, cols 7-31 - Cover the false outcome of `_cursor >= _higherCursor`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 408, cols 7-30 - Cover the true outcome of `_cursor >= _text.size()`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 421, cols 7-28 - Cover the true outcome of `_text.empty() == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 421, cols 32-44 - Cover the true outcome of `_cursor == 0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 429, cols 7-30 - Cover the true outcome of `_cursor <= _lowerCursor`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 431, cols 20-36 - Cover the true outcome and false outcome of `_lowerCursor > 0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 431, cols 40-51 - Cover the true outcome and false outcome of `_cursor > 0`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 451, cols 7-30 - Cover the false outcome of `_cursor > _higherCursor`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 468, cols 7-58 - Cover the true outcome of `p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 3}`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 523, cols 7-46 - Cover the false outcome of `sameColor(_glyphColor, p_color) == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 545, cols 7-47 - Cover the true outcome and false outcome of `sameColor(_cursorColor, p_color) == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 556, cols 7-24 - Cover the true outcome and false outcome of `_depth == p_depth`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 583, cols 7-28 - Cover the false outcome of `_text.empty() == true`.
- [ ] `srcs/structures/widget/spk_text_edit.cpp` - Line 598, cols 7-29 - Cover the true outcome of `_isObscured == p_state`.

### srcs/structures/widget/spk_text_label.cpp

Coverage: lines 100.00% (218/218, missing 0); branches 95.83% (46/48, missing 2); functions 100.00% (33/33, missing 0); regions 100.00% (96/96, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_text_label.cpp` - Line 85, cols 8-24 - Cover the false outcome of `_font != nullptr`.
- [ ] `srcs/structures/widget/spk_text_label.cpp` - Line 127, cols 7-23 - Cover the false outcome of `_font != nullptr`.

### srcs/structures/widget/spk_widget.cpp

Coverage: lines 94.96% (433/456, missing 23); branches 86.76% (59/68, missing 9); functions 88.89% (64/72, missing 8); regions 94.81% (146/154, missing 8).

Missing functions or function instantiations:

- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 42-43 - Execute the currently uncalled function: `void Widget::applyStyle(const spk::WidgetStyle &)`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 74-76 - Execute the currently uncalled function: `void Widget::_onWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event)`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 78-80 - Execute the currently uncalled function: `void Widget::_onWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event)`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 82-84 - Execute the currently uncalled function: `void Widget::_onWindowShownEvent(spk::WindowShownEvent &p_event)`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 86-88 - Execute the currently uncalled function: `void Widget::_onWindowHiddenEvent(spk::WindowHiddenEvent &p_event)`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 114-116 - Execute the currently uncalled function: `void Widget::_onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event)`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 126-128 - Execute the currently uncalled function: `void Widget::_onTextInputEvent(spk::TextInputEvent &p_event)`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Lines 242-244 - Execute the currently uncalled function: `void Widget::move(const spk::Vector2Int &p_delta)`.

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 207, cols 8-24 - Cover the true outcome of `child == nullptr`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 207, cols 60-88 - Cover the true outcome of `child->_geometry.size.y == 0`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 259, cols 7-27 - Cover the false outcome of `_viewport != nullptr`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 274, cols 8-24 - Cover the false outcome of `child != nullptr`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 300, cols 8-24 - Cover the false outcome of `child != nullptr`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 334, cols 39-61 - Cover the true outcome of `_renderUnit == nullptr`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 353, cols 7-27 - Cover the false outcome of `_viewport != nullptr`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 371, cols 8-24 - Cover the false outcome of `child != nullptr`.
- [ ] `srcs/structures/widget/spk_widget.cpp` - Line 389, cols 8-24 - Cover the false outcome of `child != nullptr`.

### srcs/structures/widget/spk_widget_style.cpp

Coverage: lines 100.00% (248/248, missing 0); branches 97.22% (35/36, missing 1); functions 100.00% (43/43, missing 0); regions 100.00% (94/94, missing 0).

Uncovered branch outcomes:

- [ ] `srcs/structures/widget/spk_widget_style.cpp` - Line 177, cols 9-27 - Cover the false outcome of `p_style != nullptr`.

### srcs/utils/spk_winapi_helpers.cpp

Coverage: lines 89.09% (49/55, missing 6); branches 62.50% (10/16, missing 6); functions 100.00% (4/4, missing 0); regions 93.33% (28/30, missing 2).

Uncovered code regions:

- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Lines 16-18 - Exercise this uncovered code path: `throwLastError("MultiByteToWideChar");`.
- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Lines 34-36 - Exercise this uncovered code path: `throwLastError("WideCharToMultiByte");`.

Uncovered branch outcomes:

- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Line 15, cols 7-16 - Cover the true outcome of `size <= 0`.
- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Line 33, cols 7-16 - Cover the true outcome of `size <= 0`.
- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Line 58, cols 7-21 - Cover the false outcome of `errorCode != 0`.
- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Line 63, cols 7-16 - Cover the false outcome of `size != 0`.
- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Line 63, cols 20-37 - Cover the false outcome of `buffer != nullptr`.
- [ ] `srcs/utils/spk_winapi_helpers.cpp` - Line 68, cols 7-24 - Cover the false outcome of `buffer != nullptr`.
