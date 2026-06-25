# Missing Test Coverage

Generated from `build/coverage/coverage-summary.json` and the current LLVM text coverage view. The report source was created on 2026-06-25 and the HTML entry point is `build/coverage/coverage-html/index.html`.

Each checklist item follows: `File - Line(s) - Missing test description`.

## Coverage Totals

- Lines: 91.96% covered (15104/16424), 1320 missing.
- Branches: 78.91% covered (3057/3874), 817 missing.
- Functions: 96.10% covered (2291/2384), 93 missing.
- Regions: 92.06% covered (6780/7365), 585 missing.
- Files with at least one missing line, branch, or function total: 101.

## Missing Points

### includes/structures/container/spk_object_pool.hpp

Coverage: lines 97.78% (132/135, missing 3); branches 94.74% (36/38, missing 2); functions 100.00% (18/18, missing 0).

- `includes/structures/container/spk_object_pool.hpp` - Line 231 (cols 8-24) - Add branch coverage for `_data == nullptr`; missing true outcome.

### includes/structures/design_pattern/spk_contract_provider.hpp

Coverage: lines 95.80% (137/143, missing 6); branches 83.33% (35/42, missing 7); functions 100.00% (22/22, missing 0).

- `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 141-143 - Add a test that reaches this return path: `return;`.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Lines 158-160 - Add a test that reaches this return path: `return;`.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 94 (cols 9-25) - Add branch coverage for `this != &p_other`; missing false outcome in 12 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 105 (cols 9-25) - Add branch coverage for `_link != nullptr`; missing false outcome in 6 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 119 (cols 13-29) - Add branch coverage for `_link != nullptr`; missing true outcome and false outcome in 5 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 119 (cols 33-59) - Add branch coverage for `_link->function != nullptr`; missing true outcome and false outcome in 6 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 124 (cols 9-25) - Add branch coverage for `_link == nullptr`; missing true outcome.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 140 (cols 8-21) - Add branch coverage for `_isTriggering`; missing true outcome in 20 counters and false outcome.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 150 (cols 15-32) - Add branch coverage for `p_link == nullptr`; missing true outcome in 20 counters and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 150 (cols 36-63) - Add branch coverage for `p_link->function == nullptr`; missing true outcome in 17 counters and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 157 (cols 8-50) - Add branch coverage for `_lastTriggerArguments.has_value() == false`; missing true outcome in 20 counters and false outcome in 18 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 194 (cols 8-19) - Add branch coverage for `isBlocked`; missing true outcome in 18 counters and false outcome.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 196 (cols 9-25) - Add branch coverage for `isDelayBlocked`; missing true outcome in 18 counters and false outcome in 18 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 207 (cols 8-21) - Add branch coverage for `_isTriggering`; missing true outcome in 18 counters and false outcome.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 217 (cols 24-40) - Add branch coverage for `i < initialCount`; missing true outcome in 2 counters and false outcome.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 221 (cols 10-28) - Add branch coverage for `element == nullptr`; missing true outcome in 20 counters and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 222 (cols 7-35) - Add branch coverage for `element->function == nullptr`; missing true outcome in 18 counters and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 223 (cols 7-27) - Add branch coverage for `element->isBlocked`; missing true outcome in 18 counters and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 239 (cols 9-27) - Add branch coverage for `element != nullptr`; missing false outcome.
- `includes/structures/design_pattern/spk_contract_provider.hpp` - Line 254 (cols 9-27) - Add branch coverage for `element != nullptr`; missing false outcome.

### includes/structures/design_pattern/spk_inherence_trait.hpp

Coverage: lines 93.91% (293/312, missing 19); branches 80.77% (84/104, missing 20); functions 100.00% (37/37, missing 0).

- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 98-100 - Add a test that reaches the false-return path: `return false;`.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 118-120 - Add a test that reaches the false-return path: `return false;`.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 144-146 - Add a test that reaches this return path: `return;`.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 171-173 - Add a test that reaches this return path: `return;`.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 251-254 - Add a loop/control-flow test that reaches this block: `continue;`.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Lines 270-272 - Add a test that reaches this return path: `return;`.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 57 (cols 12-38) - Add branch coverage for `_traversalState != nullptr`; missing false outcome in 4 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 57 (cols 42-72) - Add branch coverage for `_traversalState->nbBlocks != 0`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 67 (cols 8-25) - Add branch coverage for `p_node != nullptr`; missing true outcome in 2 counters and false outcome in 4 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 72 (cols 8-27) - Add branch coverage for `p_parent != nullptr`; missing true outcome in 2 counters and false outcome in 3 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 87 (cols 8-25) - Add branch coverage for `p_node != nullptr`; missing true outcome and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 97 (cols 8-34) - Add branch coverage for `p_mutation.node == nullptr`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 102 (cols 8-51) - Add branch coverage for `p_mutation.nodeAliveToken.expired() == true`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 112 (cols 8-36) - Add branch coverage for `p_mutation.parent == nullptr`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 117 (cols 8-47) - Add branch coverage for `p_mutation.hasParentAliveToken == false`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 127 (cols 12-54) - Add branch coverage for `_isMutationTargetAlive(p_mutation) == true`; missing false outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 127 (cols 58-100) - Add branch coverage for `_isMutationParentAlive(p_mutation) == true`; missing false outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 138 (cols 12-48) - Add branch coverage for `trait->_isTraversalBlocked() == true`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 143 (cols 8-34) - Add branch coverage for `_traversalState == nullptr`; missing true outcome in 4 counters and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 148 (cols 8-58) - Add branch coverage for `p_mutation.type == DeferredMutationType::SetParent`; missing true outcome in 2 counters and false outcome in 3 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 154 (cols 14-72) - Add branch coverage for `p_existingMutation.type == DeferredMutationType::SetParent`; missing true outcome in 3 counters and false outcome in 4 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 155 (cols 11-53) - Add branch coverage for `p_existingMutation.node == p_mutation.node`; missing true outcome in 3 counters and false outcome in 4 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 158 (cols 9-61) - Add branch coverage for `iterator != _traversalState->deferredMutations.end`; missing true outcome in 3 counters and false outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 170 (cols 8-34) - Add branch coverage for `_traversalState == nullptr`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 182 (cols 10-45) - Add branch coverage for `_isMutationValid(mutation) == false`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 187 (cols 14-27) - Add branch coverage for `mutation.type`; missing false outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 189 (cols 6-42) - Add branch coverage for `case DeferredMutationType::SetParent`; missing false outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 193 (cols 6-46) - Add branch coverage for `case DeferredMutationType::ClearChildren`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 205 (cols 8-24) - Add branch coverage for `p_parent == self`; missing true outcome in 3 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 215 (cols 31-67) - Add branch coverage for `self->isAncestorOf(p_parent) == true`; missing true outcome in 3 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 225 (cols 9-48) - Add branch coverage for `oldIterator != _parent->_children.end`; missing false outcome in 4 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 246 (cols 11-37) - Add branch coverage for `_children.empty() == false`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 250 (cols 9-25) - Add branch coverage for `child == nullptr`; missing true outcome in 4 counters and false outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 269 (cols 9-27) - Add branch coverage for `p_owner == nullptr`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 269 (cols 31-66) - Add branch coverage for `p_owner->_traversalState == nullptr`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 291 (cols 9-25) - Add branch coverage for `this != &p_other`; missing false outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 310 (cols 9-25) - Add branch coverage for `state == nullptr`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 315 (cols 9-29) - Add branch coverage for `state->nbBlocks != 0`; missing false outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 321 (cols 9-29) - Add branch coverage for `state->nbBlocks == 0`; missing false outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 321 (cols 33-49) - Add branch coverage for `owner != nullptr`; missing false outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 353 (cols 8-34) - Add branch coverage for `_traversalState != nullptr`; missing false outcome in 4 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 362 (cols 8-34) - Add branch coverage for `_traversalState != nullptr`; missing false outcome in 4 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 401 (cols 9-52) - Add branch coverage for `current == static_cast<const TType *>(this`; missing true outcome in 3 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 425 (cols 8-31) - Add branch coverage for `blockedOwner == nullptr`; missing false outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 430 (cols 8-31) - Add branch coverage for `blockedOwner != nullptr`; missing true outcome in 2 counters.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 441 (cols 8-26) - Add branch coverage for `p_child == nullptr`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 451 (cols 8-26) - Add branch coverage for `p_child == nullptr`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 456 (cols 8-54) - Add branch coverage for `p_child->_parent != static_cast<TType *>(this`; missing true outcome.
- `includes/structures/design_pattern/spk_inherence_trait.hpp` - Line 467 (cols 8-31) - Add branch coverage for `blockedOwner != nullptr`; missing true outcome and false outcome.

### includes/structures/game_engine/spk_component_container.hpp

Coverage: lines 100.00% (34/34, missing 0); branches 91.67% (11/12, missing 1); functions 100.00% (7/7, missing 0).

- `includes/structures/game_engine/spk_component_container.hpp` - Line 47 (cols 9-29) - Add branch coverage for `component != nullptr`; missing false outcome.

### includes/structures/game_engine/spk_component_logic.hpp

Coverage: lines 94.83% (220/232, missing 12); branches 67.86% (19/28, missing 9); functions 100.00% (85/85, missing 0).

- `includes/structures/game_engine/spk_component_logic.hpp` - Lines 84-86 - Add a test that reaches this return path: `return;`.
- `includes/structures/game_engine/spk_component_logic.hpp` - Lines 106-108 - Add a test that reaches this return path: `return;`.
- `includes/structures/game_engine/spk_component_logic.hpp` - Lines 126-128 - Add a test that reaches this return path: `return;`.
- `includes/structures/game_engine/spk_component_logic.hpp` - Lines 148-150 - Add a test that reaches this return path: `return;`.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 83 (cols 8-30) - Add branch coverage for `isActivated() == false`; missing true outcome in 108 counters and false outcome in 87 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 83 (cols 34-62) - Add branch coverage for `p_event.isConsumed() == true`; missing true outcome in 108 counters and false outcome in 87 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 90 (cols 35-36) - Add branch coverage for `for (spk::Component *component : p_registry.container<TComponent>().processableComponents())`; missing true outcome in 88 counters and false outcome in 87 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 92 (cols 9-29) - Add branch coverage for `component == nullptr`; missing true outcome in 108 counters and false outcome in 88 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 92 (cols 33-61) - Add branch coverage for `p_event.isConsumed() == true`; missing true outcome in 107 counters and false outcome in 88 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 105 (cols 8-30) - Add branch coverage for `isActivated() == false`; missing true outcome in 6 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 112 (cols 35-36) - Add branch coverage for `for (spk::Component *component : p_registry.container<TComponent>().processableComponents())`; missing true outcome in 3 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 114 (cols 9-29) - Add branch coverage for `component != nullptr`; missing true outcome in 3 counters and false outcome in 6 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 125 (cols 8-30) - Add branch coverage for `isActivated() == false`; missing true outcome in 6 counters and false outcome in 5 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 132 (cols 35-36) - Add branch coverage for `for (spk::Component *component : p_registry.container<TComponent>().processableComponents())`; missing true outcome in 5 counters and false outcome in 5 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 134 (cols 9-29) - Add branch coverage for `component != nullptr`; missing true outcome in 5 counters and false outcome in 6 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 147 (cols 8-30) - Add branch coverage for `isActivated() == false`; missing true outcome in 6 counters and false outcome in 4 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 154 (cols 35-36) - Add branch coverage for `for (spk::Component *component : p_registry.container<TComponent>().processableComponents())`; missing true outcome in 4 counters and false outcome in 4 counters.
- `includes/structures/game_engine/spk_component_logic.hpp` - Line 156 (cols 9-29) - Add branch coverage for `component != nullptr`; missing true outcome in 4 counters and false outcome in 6 counters.

### includes/structures/game_engine/spk_component_logic_registry.hpp

Coverage: lines 100.00% (85/85, missing 0); branches 84.38% (27/32, missing 5); functions 100.00% (12/12, missing 0).

- `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 109 (cols 9-25) - Add branch coverage for `logic != nullptr`; missing false outcome.
- `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 122 (cols 9-25) - Add branch coverage for `logic != nullptr`; missing false outcome.
- `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 137 (cols 9-25) - Add branch coverage for `logic != nullptr`; missing false outcome.
- `includes/structures/game_engine/spk_component_logic_registry.hpp` - Line 153 (cols 9-25) - Add branch coverage for `logic == nullptr`; missing true outcome in 18 counters.

### includes/structures/graphics/geometry/spk_generic_mesh.hpp

Coverage: lines 97.09% (100/103, missing 3); branches 86.36% (19/22, missing 3); functions 100.00% (20/20, missing 0).

- `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Lines 44-46 - Add a negative-path test that triggers and asserts this exception: `spk::GenericMesh: too many vertices for uint32_t indexes`.
- `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 43 (cols 8-37) - Add branch coverage for `currentVertexCount > maxIndex`; missing true outcome in 3 counters.
- `includes/structures/graphics/geometry/spk_generic_mesh.hpp` - Line 43 (cols 41-90) - Add branch coverage for `p_vertexCount - 1 > maxIndex - currentVertexCount`; missing true outcome in 3 counters.

### includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp

Coverage: lines 78.71% (122/155, missing 33); branches 61.36% (27/44, missing 17); functions 83.33% (15/18, missing 3).

- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 74-77 - Add a test that reaches this return path: `return p_entry.contextId != p_currentContextId &&`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 80-84 - Add a test that reaches this uncovered block: `if (p_index != _entries.size() - 1)`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 86-87 - Add a test that reaches this uncovered block: `_entries.pop_back();`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 94-96 - Add a test that reaches this uncovered block: `*p_entry = std::move(_entries.back());`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 110-120 - Add a test that reaches this uncovered block: `for (std::size_t index = 0; index < _entries.size();)`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 122-123 - Add a test that reaches the true-return path: `return true;`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Lines 234-236 - Add a test that reaches this uncovered block: `entry = _findEntry(contextId);`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 265 - Add a test that reaches this uncovered block: `});`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 90 - Add a test that calls this currently unexecuted function (2 function counters start here): `void _removeEntry(Entry *p_entry)`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 264 - Add a test that calls this currently unexecuted function (5 function counters start here): `[](TGpuObject &) {`.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 42 (cols 9-39) - Add branch coverage for `entry.contextId == p_contextId`; missing false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 53 (cols 28-29) - Add branch coverage for `for (const Entry &entry : _entries)`; missing false outcome in 3 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 55 (cols 9-39) - Add branch coverage for `entry.contextId == p_contextId`; missing false outcome in 4 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 69 (cols 33-72) - Add branch coverage for `p_entry->object->version() == p_version`; missing false outcome in 2 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 70 (cols 8-61) - Add branch coverage for `p_entry->object->contentVersion() == p_contentVersion`; missing false outcome in 4 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 75 (cols 11-50) - Add branch coverage for `p_entry.contextId != p_currentContextId`; missing true outcome in 5 counters and false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 76 (cols 8-63) - Add branch coverage for `spk::OpenGL::isContextAlive(p_entry.contextId) == false`; missing true outcome in 5 counters and false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 81 (cols 8-38) - Add branch coverage for `p_index != _entries.size() - 1`; missing true outcome in 5 counters and false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 93 (cols 8-35) - Add branch coverage for `p_entry != &_entries.back`; missing true outcome in 5 counters and false outcome in 2 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 105 (cols 8-48) - Add branch coverage for `currentGeneration == _prunedAtGeneration`; missing false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 110 (cols 32-55) - Add branch coverage for `index < _entries.size`; missing true outcome in 5 counters and false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 112 (cols 9-75) - Add branch coverage for `_isDeadForeignContext(_entries[index], p_currentContextId) == true`; missing true outcome in 5 counters and false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 148 (cols 8-26) - Add branch coverage for `p_entry != nullptr`; missing true outcome in 2 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 148 (cols 30-69) - Add branch coverage for `p_entry->object->version() != p_version`; missing true outcome in 3 counters and false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 154 (cols 8-26) - Add branch coverage for `p_entry == nullptr`; missing false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 172 (cols 8-60) - Add branch coverage for `p_entry.object->contentVersion() == p_contentVersion`; missing false outcome in 5 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 233 (cols 8-52) - Add branch coverage for `_pruneDeadForeignContexts(contextId) == true`; missing true outcome in 6 counters.
- `includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp` - Line 273 (cols 8-24) - Add branch coverage for `entry == nullptr`; missing true outcome in 3 counters.

### includes/structures/widget/spk_grid_layout.hpp

Coverage: lines 83.89% (380/453, missing 73); branches 76.70% (135/176, missing 41); functions 92.86% (26/28, missing 2).

- `includes/structures/widget/spk_grid_layout.hpp` - Lines 29-31 - Add a test that reaches this return path: `return;`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 108 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 136 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 143-145 - Add a test that reaches this return path: `return {0u, 0u};`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 174-184 - Add a switch/enum test case that reaches this branch: `case SizePolicy::Fixed:`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 193 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 213-224 - Add a switch/enum test case that reaches this branch: `case SizePolicy::Fixed:`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 236 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 256-257 - Add a test that reaches this return path: `return _computeMinimalSize();`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 261-263 - Add a test that reaches this return path: `return _computePreferredSizeFor(p_availableSize);`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 305-308 - Add a test that reaches this uncovered block: `*_elements[index] = Element(p_widget, p_sizePolicy, spk::Vector2UInt{1, 1});`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 339-341 - Add a loop/control-flow test that reaches this block: `continue;`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 358 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 367 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 376 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 380 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 391 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 398 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 408 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 441-443 - Add a loop/control-flow test that reaches this block: `continue;`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 452 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 458 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 469 - Add a test that reaches this uncovered block: `}`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 521-523 - Add a test that reaches this return path: `return;`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 540-541 - Add a test that reaches this return path: `return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 552-554 - Add a test that reaches this return path: `return;`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 571-572 - Add a test that reaches this return path: `return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);`.
- `includes/structures/widget/spk_grid_layout.hpp` - Lines 583-585 - Add a test that reaches this return path: `return;`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 608 - Add a test that reaches this return path: `return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 255 - Add a test that calls this currently unexecuted function: `configureFixedSizeGenerator([this]() {`.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 28 (cols 29-49) - Add branch coverage for `p_columns == _size.x`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 142 (cols 8-20) - Add branch coverage for `_size.y == 0`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 142 (cols 24-36) - Add branch coverage for `_size.x == 0`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 155 (cols 5-40) - Add branch coverage for `p_availableSize.y > totalPaddingY`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 163 (cols 33-61) - Add branch coverage for `element->widget() == nullptr`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 163 (cols 65-93) - Add branch coverage for `element->layout() == nullptr`; missing true outcome and false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 174 (cols 6-28) - Add branch coverage for `case SizePolicy::Fixed`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 180 (cols 6-30) - Add branch coverage for `case SizePolicy::Maximum`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 185 (cols 6-13) - Add branch coverage for `default`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 204 (cols 33-61) - Add branch coverage for `element->widget() == nullptr`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 204 (cols 65-93) - Add branch coverage for `element->layout() == nullptr`; missing true outcome and false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 213 (cols 6-28) - Add branch coverage for `case SizePolicy::Fixed`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 219 (cols 6-30) - Add branch coverage for `case SizePolicy::Maximum`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 225 (cols 6-13) - Add branch coverage for `default`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 283 (cols 29-43) - Add branch coverage for `_size.x == 0`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 288 (cols 16-30) - Add branch coverage for `_size.y == 0`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 301 (cols 8-35) - Add branch coverage for `_elements[index] == nullptr`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 314 (cols 24-36) - Add branch coverage for `_size.y == 0`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 322 (cols 5-40) - Add branch coverage for `p_geometry.size.x > totalPaddingX`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 338 (cols 10-28) - Add branch coverage for `element == nullptr`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 338 (cols 33-61) - Add branch coverage for `element->widget() == nullptr`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 338 (cols 65-93) - Add branch coverage for `element->layout() == nullptr`; missing true outcome and false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 361 (cols 11-38) - Add branch coverage for `isExtendableOnX[x] == false`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 370 (cols 11-38) - Add branch coverage for `isExtendableOnY[y] == false`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 413 (cols 24-59) - Add branch coverage for `p_geometry.size.x > totalPaddingX`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 421 (cols 17-43) - Add branch coverage for `spaceLeftX >= sumColsMin`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 440 (cols 10-28) - Add branch coverage for `element == nullptr`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 440 (cols 33-61) - Add branch coverage for `element->widget() == nullptr`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 440 (cols 65-93) - Add branch coverage for `element->layout() == nullptr`; missing true outcome and false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 474 (cols 24-59) - Add branch coverage for `p_geometry.size.y > totalPaddingY`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 481 (cols 17-43) - Add branch coverage for `spaceLeftY >= sumRowsMin`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 500 (cols 10-28) - Add branch coverage for `element != nullptr`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 520 (cols 8-22) - Add branch coverage for `NbColumns == 0`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 536 (cols 8-29) - Add branch coverage for `p_column >= NbColumns`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 551 (cols 8-19) - Add branch coverage for `NbRows == 0`; missing true outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 567 (cols 8-23) - Add branch coverage for `p_row >= NbRows`; missing false outcome.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 582 (cols 8-22) - Add branch coverage for `NbColumns == 0`; missing true outcome in 2 counters and false outcome in 2 counters.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 582 (cols 26-37) - Add branch coverage for `NbRows == 0`; missing true outcome in 2 counters and false outcome in 2 counters.
- `includes/structures/widget/spk_grid_layout.hpp` - Line 604 (cols 33-48) - Add branch coverage for `p_row >= NbRows`; missing false outcome.

### includes/structures/widget/spk_linear_layout.hpp

Coverage: lines 94.96% (245/258, missing 13); branches 88.89% (64/72, missing 8); functions 95.65% (22/23, missing 1).

- `includes/structures/widget/spk_linear_layout.hpp` - Lines 191-193 - Add a loop/control-flow test that reaches this block: `continue;`.
- `includes/structures/widget/spk_linear_layout.hpp` - Lines 330-332 - Add a test that reaches this return path: `return {0u, 0u};`.
- `includes/structures/widget/spk_linear_layout.hpp` - Lines 381-382 - Add a test that reaches this return path: `return _computeMinimalSize();`.
- `includes/structures/widget/spk_linear_layout.hpp` - Lines 399-401 - Add a test that reaches this return path: `return;`.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 380 - Add a test that calls this currently unexecuted function (2 function counters start here): `configureFixedSizeGenerator([this]() {`.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 58 (cols 11-31) - Add branch coverage for `p_element != nullptr`; missing false outcome in 2 counters.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 58 (cols 36-66) - Add branch coverage for `p_element->widget() != nullptr`; missing false outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 58 (cols 70-100) - Add branch coverage for `p_element->layout() != nullptr`; missing true outcome and false outcome in 2 counters.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 120 (cols 12-24) - Add branch coverage for `p_sizePolicy`; missing false outcome in 2 counters.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 122 (cols 4-26) - Add branch coverage for `case SizePolicy::Fixed`; missing true outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 130 (cols 4-28) - Add branch coverage for `case SizePolicy::Maximum`; missing true outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 182 (cols 47-73) - Add branch coverage for `_elements.empty() == false`; missing false outcome in 2 counters.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 190 (cols 9-42) - Add branch coverage for `_isValidElement(element) == false`; missing true outcome in 2 counters.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 214 (cols 24-47) - Add branch coverage for `p_items.empty() == true`; missing true outcome in 2 counters.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 228 (cols 8-24) - Add branch coverage for `extendCount == 0`; missing true outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 230 (cols 19-20) - Add branch coverage for `for (auto &it : p_items)`; missing true outcome and false outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 247 (cols 24-34) - Add branch coverage for `remain > 0`; missing true outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 248 (cols 9-19) - Add branch coverage for `remain > 0`; missing true outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 323 (cols 8-33) - Add branch coverage for `_elements.empty() == true`; missing true outcome.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 329 (cols 8-29) - Add branch coverage for `items.empty() == true`; missing true outcome in 2 counters.
- `includes/structures/widget/spk_linear_layout.hpp` - Line 398 (cols 8-29) - Add branch coverage for `items.empty() == true`; missing true outcome in 2 counters.

### includes/structures/widget/spk_numeric_spin_box.hpp

Coverage: lines 87.12% (142/163, missing 21); branches 50.00% (17/34, missing 17); functions 95.00% (19/20, missing 1).

- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 51-56 - Add a test that reaches the true-return path: `return true;`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 58 - Add a test that reaches the false-return path: `return false;`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 93-96 - Add a test that reaches this uncovered block: `std::istringstream stream(p_text);`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 98-101 - Add a test that reaches this return path: `return ValidationState::Invalid;`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 103-107 - Add a test that reaches this return path: `return ValidationState::Invalid;`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 109-111 - Add a test that reaches this return path: `return ValidationState::Valid;`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 170-172 - Add a test that reaches this return path: `return ValidationState::Invalid;`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 195-197 - Add a test that reaches this uncovered block: `const spk::Vector2UInt editSize = _valueEdit.minimalSize();`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Lines 199-202 - Add a test that reaches this return path: `return spk::Vector2UInt(`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 44 - Add a test that calls this currently unexecuted function: `[[nodiscard]] static bool _isEmptyOrSignOnly(const std::string &p_text)`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 62 - Add a test that calls this currently unexecuted function: `[[nodiscard]] static ValidationState _parseValue(const std::string &p_text, value_type &p_outValue)`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 164 - Add a test that calls this currently unexecuted function: `[](const spk::Font::Text &p_text) {`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 184 - Add a test that calls this currently unexecuted function (2 function counters start here): `_raiseContract = _raiseButton.subscribeToClick([this]() {`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 188 - Add a test that calls this currently unexecuted function (2 function counters start here): `_lowerContract = _lowerButton.subscribeToClick([this]() {`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 194 - Add a test that calls this currently unexecuted function (3 function counters start here): `configureMinimalSizeGenerator([this]() {`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 213 - Add a test that calls this currently unexecuted function: `void setValue(const value_type &p_value)`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 234 - Add a test that calls this currently unexecuted function: `void increase()`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 239 - Add a test that calls this currently unexecuted function (2 function counters start here): `void decrease()`.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45 (cols 8-30) - Add branch coverage for `p_text.empty() == true`; missing true outcome in 2 counters and false outcome in 2 counters.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45 (cols 34-47) - Add branch coverage for `p_text == "-"`; missing true outcome in 3 counters and false outcome in 2 counters.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 45 (cols 51-64) - Add branch coverage for `p_text == "+"`; missing true outcome in 3 counters and false outcome in 2 counters.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 52 (cols 9-22) - Add branch coverage for `p_text == "."`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 52 (cols 26-40) - Add branch coverage for `p_text == "-."`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 52 (cols 44-58) - Add branch coverage for `p_text == "+."`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 65 (cols 36-57) - Add branch coverage for `p_text.front() == '-'`; missing false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 71 (cols 8-42) - Add branch coverage for `_isEmptyOrSignOnly(p_text) == true`; missing true outcome in 2 counters and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 80 (cols 30-53) - Add branch coverage for `p_text.front() == '+'`; missing true outcome in 2 counters.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 84 (cols 9-26) - Add branch coverage for `ec != std::errc`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 84 (cols 30-40) - Add branch coverage for `ptr != end`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 98 (cols 9-30) - Add branch coverage for `stream.fail() == true`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 104 (cols 9-30) - Add branch coverage for `stream.eof() == false`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 128 (cols 8-33) - Add branch coverage for `_isRefreshingText == true`; missing false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 134 (cols 8-83) - Add branch coverage for `_parseValue(_valueEdit.textAsUTF8(), parsedValue) == ValidationState::Valid`; missing true outcome in 2 counters and false outcome in 2 counters.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 145 (cols 5-42) - Add branch coverage for `geometry().width() > buttonSize * 2`; missing false outcome in 3 counters.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 167 (cols 30-31) - Add branch coverage for `for (char32_t codepoint : p_text)`; missing true outcome and false outcome.
- `includes/structures/widget/spk_numeric_spin_box.hpp` - Line 169 (cols 11-26) - Add branch coverage for `codepoint > 127`; missing true outcome in 3 counters and false outcome.

### includes/structures/widget/spk_spin_box.hpp

Coverage: lines 93.10% (108/116, missing 8); branches 75.00% (6/8, missing 2); functions 95.00% (19/20, missing 1).

- `includes/structures/widget/spk_spin_box.hpp` - Lines 101-103 - Add a test that reaches this uncovered block: `const spk::Vector2UInt downSize = _downButton.minimalSize();`.
- `includes/structures/widget/spk_spin_box.hpp` - Lines 105-108 - Add a test that reaches this return path: `return spk::Vector2UInt(`.
- `includes/structures/widget/spk_spin_box.hpp` - Line 56 - Add a test that calls this currently unexecuted function: `void _onGeometryChange() override`.
- `includes/structures/widget/spk_spin_box.hpp` - Line 78 - Add a test that calls this currently unexecuted function: `_downButtonContract = _downButton.subscribeToClick([this]() {`.
- `includes/structures/widget/spk_spin_box.hpp` - Line 82 - Add a test that calls this currently unexecuted function: `_upButtonContract = _upButton.subscribeToClick([this]() {`.
- `includes/structures/widget/spk_spin_box.hpp` - Line 100 - Add a test that calls this currently unexecuted function (2 function counters start here): `configureMinimalSizeGenerator([this]() {`.
- `includes/structures/widget/spk_spin_box.hpp` - Line 43 (cols 8-37) - Add branch coverage for `_minLimit.has_value() == true`; missing true outcome.
- `includes/structures/widget/spk_spin_box.hpp` - Line 47 (cols 8-37) - Add branch coverage for `_maxLimit.has_value() == true`; missing true outcome.
- `includes/structures/widget/spk_spin_box.hpp` - Line 59 (cols 5-42) - Add branch coverage for `geometry().width() > buttonSize * 2`; missing true outcome and false outcome in 2 counters.
- `includes/structures/widget/spk_spin_box.hpp` - Line 92 (cols 8-26) - Add branch coverage for `iconset != nullptr`; missing false outcome in 2 counters.

### includes/structures/widget/spk_widget.hpp

Coverage: lines 100.00% (19/19, missing 0); branches 80.00% (8/10, missing 2); functions 100.00% (1/1, missing 0).

- `includes/structures/widget/spk_widget.hpp` - Line 71 (cols 8-30) - Add branch coverage for `isActivated() == false`; missing true outcome in 11 counters.
- `includes/structures/widget/spk_widget.hpp` - Line 71 (cols 34-62) - Add branch coverage for `p_event.isConsumed() == true`; missing true outcome in 18 counters.
- `includes/structures/widget/spk_widget.hpp` - Line 76 (cols 21-22) - Add branch coverage for `for (auto *child : children())`; missing true outcome in 7 counters.
- `includes/structures/widget/spk_widget.hpp` - Line 78 (cols 9-25) - Add branch coverage for `child != nullptr`; missing true outcome in 7 counters and false outcome in 18 counters.
- `includes/structures/widget/spk_widget.hpp` - Line 81 (cols 10-38) - Add branch coverage for `p_event.isConsumed() == true`; missing true outcome in 15 counters and false outcome in 8 counters.

### includes/structures/widget/spk_workspace.hpp

Coverage: lines 100.00% (34/34, missing 0); branches 50.00% (1/2, missing 1); functions 100.00% (5/5, missing 0).

- `includes/structures/widget/spk_workspace.hpp` - Line 29 (cols 5-46) - Add branch coverage for `geometry().height() > _menuBar.height`; missing false outcome.

### srcs/structures/application/module/spk_keyboard_module.cpp

Coverage: lines 93.33% (42/45, missing 3); branches 100.00% (4/4, missing 0); functions 88.89% (8/9, missing 1).

- `srcs/structures/application/module/spk_keyboard_module.cpp` - Lines 53-55 - Add a test that reaches this return path: `return _keyboard;`.

### srcs/structures/application/module/spk_mouse_module.cpp

Coverage: lines 95.89% (70/73, missing 3); branches 100.00% (4/4, missing 0); functions 92.31% (12/13, missing 1).

- `srcs/structures/application/module/spk_mouse_module.cpp` - Lines 67-69 - Add a test that reaches this return path: `return _mouse;`.

### srcs/structures/application/module/spk_render_module.cpp

Coverage: lines 82.35% (14/17, missing 3); branches 50.00% (1/2, missing 1); functions 100.00% (4/4, missing 0).

- `srcs/structures/application/module/spk_render_module.cpp` - Lines 26-28 - Add a test that reaches this return path: `return;`.
- `srcs/structures/application/module/spk_render_module.cpp` - Line 25 (cols 7-26) - Add branch coverage for `snapshot == nullptr`; missing true outcome.

### srcs/structures/application/spk_application.cpp

Coverage: lines 93.66% (251/268, missing 17); branches 82.89% (63/76, missing 13); functions 100.00% (21/21, missing 0).

- `srcs/structures/application/spk_application.cpp` - Lines 85-87 - Add a test that reaches this return path: `return;`.
- `srcs/structures/application/spk_application.cpp` - Lines 129-131 - Add a test that reaches this return path: `return;`.
- `srcs/structures/application/spk_application.cpp` - Lines 176-177 - Add a loop/control-flow test that reaches this block: `continue;`.
- `srcs/structures/application/spk_application.cpp` - Lines 275-277 - Add a test that reaches this uncovered block: `_platformRuntime = _createDefaultPlatformRuntime();`.
- `srcs/structures/application/spk_application.cpp` - Lines 280-282 - Add a test that reaches this uncovered block: `_gpuPlatformRuntime = _createDefaultGPUPlatformRuntime();`.
- `srcs/structures/application/spk_application.cpp` - Lines 322-324 - Add a test that reaches this uncovered block: `_recordFailure(std::current_exception());`.
- `srcs/structures/application/spk_application.cpp` - Line 75 (cols 6-46) - Add branch coverage for `_configuration.stopWhenNoWindows == true`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 84 (cols 10-39) - Add branch coverage for `_stopRequested.load() == true`; missing true outcome.
- `srcs/structures/application/spk_application.cpp` - Line 90 (cols 10-27) - Add branch coverage for `window != nullptr`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 119 (cols 6-46) - Add branch coverage for `_configuration.stopWhenNoWindows == true`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 128 (cols 10-39) - Add branch coverage for `_stopRequested.load() == true`; missing true outcome.
- `srcs/structures/application/spk_application.cpp` - Line 134 (cols 10-27) - Add branch coverage for `window != nullptr`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 161 (cols 10-27) - Add branch coverage for `window != nullptr`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 170 (cols 9-42) - Add branch coverage for `_shutdownRequested.load() == true`; missing true outcome.
- `srcs/structures/application/spk_application.cpp` - Line 171 (cols 6-46) - Add branch coverage for `_configuration.stopWhenNoWindows == true`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 186 (cols 9-26) - Add branch coverage for `window != nullptr`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 195 (cols 43-83) - Add branch coverage for `_configuration.stopWhenNoWindows == true`; missing false outcome.
- `srcs/structures/application/spk_application.cpp` - Line 274 (cols 38-65) - Add branch coverage for `_windowRegistry.size() != 0`; missing true outcome.
- `srcs/structures/application/spk_application.cpp` - Line 279 (cols 41-68) - Add branch coverage for `_windowRegistry.size() != 0`; missing true outcome.

### srcs/structures/container/spk_binary_field.cpp

Coverage: lines 98.31% (174/177, missing 3); branches 89.58% (43/48, missing 5); functions 100.00% (21/21, missing 0).

- `srcs/structures/container/spk_binary_field.cpp` - Line 123 (cols 32-69) - Add branch coverage for `_sectionID < _layout->sections.size`; missing false outcome.

### srcs/structures/design_pattern/spk_blockable_trait.cpp

Coverage: lines 86.73% (98/113, missing 15); branches 75.00% (27/36, missing 9); functions 100.00% (14/14, missing 0).

- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 22-24 - Add a test that reaches this return path: `return;`.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 131-133 - Add a test that reaches the false-return path: `return false;`.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 141-143 - Add a test that reaches the false-return path: `return false;`.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Lines 151-153 - Add a test that reaches the false-return path: `return false;`.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 13 (cols 7-24) - Add branch coverage for `_state != nullptr`; missing false outcome.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 21 (cols 7-24) - Add branch coverage for `_state == nullptr`; missing true outcome.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 99 (cols 9-33) - Add branch coverage for `state->nbDelayBlocks > 0`; missing false outcome.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 106 (cols 9-34) - Add branch coverage for `state->nbIgnoreBlocks > 0`; missing false outcome.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 130 (cols 7-24) - Add branch coverage for `_state == nullptr`; missing true outcome.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 140 (cols 7-24) - Add branch coverage for `_state == nullptr`; missing true outcome.
- `srcs/structures/design_pattern/spk_blockable_trait.cpp` - Line 150 (cols 7-24) - Add branch coverage for `_state == nullptr`; missing true outcome.

### srcs/structures/design_pattern/spk_synchronizable_trait.cpp

Coverage: lines 89.29% (25/28, missing 3); branches 75.00% (3/4, missing 1); functions 100.00% (4/4, missing 0).

- `srcs/structures/design_pattern/spk_synchronizable_trait.cpp` - Lines 25-27 - Add a test that reaches this return path: `return;`.
- `srcs/structures/design_pattern/spk_synchronizable_trait.cpp` - Line 24 (cols 7-53) - Add branch coverage for `_needsSynchronization.exchange(false) == false`; missing true outcome.

### srcs/structures/game_engine/spk_entity.cpp

Coverage: lines 89.52% (94/105, missing 11); branches 78.57% (22/28, missing 6); functions 86.67% (13/15, missing 2).

- `srcs/structures/game_engine/spk_entity.cpp` - Lines 33-35 - Add a test that reaches this return path: `return;`.
- `srcs/structures/game_engine/spk_entity.cpp` - Lines 38-43 - Add a test that reaches this uncovered block: `for (const std::unique_ptr<spk::Component> &component : _components)`.
- `srcs/structures/game_engine/spk_entity.cpp` - Line 32 (cols 7-30) - Add branch coverage for `_registry == p_registry`; missing true outcome.
- `srcs/structures/game_engine/spk_entity.cpp` - Line 37 (cols 7-27) - Add branch coverage for `_registry != nullptr`; missing true outcome.
- `srcs/structures/game_engine/spk_entity.cpp` - Line 39 (cols 58-59) - Add branch coverage for `for (const std::unique_ptr<spk::Component> &component : _components)`; missing true outcome and false outcome.
- `srcs/structures/game_engine/spk_entity.cpp` - Line 57 (cols 8-24) - Add branch coverage for `child != nullptr`; missing false outcome.

### srcs/structures/graphics/opengl/spk_opengl_buffer.cpp

Coverage: lines 91.67% (33/36, missing 3); branches 75.00% (6/8, missing 2); functions 80.00% (4/5, missing 1).

- `srcs/structures/graphics/opengl/spk_opengl_buffer.cpp` - Line 17 (cols 8-16) - Add branch coverage for `_id != 0`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_buffer.cpp` - Line 17 (cols 20-49) - Add branch coverage for `_ownsCurrentContext() == true`; missing false outcome.

### srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp

Coverage: lines 94.51% (155/164, missing 9); branches 75.00% (27/36, missing 9); functions 92.86% (26/28, missing 2).

- `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 28 (cols 7-21) - Add branch coverage for `ctx != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 28 (cols 25-62) - Add branch coverage for `ctx->supportsOpenGLCommands() == true`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp` - Line 85 (cols 7-45) - Add branch coverage for `object->version() == _structureVersion`; missing false outcome.

### srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp

Coverage: lines 89.59% (198/221, missing 23); branches 80.43% (74/92, missing 18); functions 96.30% (26/27, missing 1).

- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 35 - Add a negative-path test that triggers and asserts this exception: `spk::LayoutBufferObject::Attribute has an unsupported component count`.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 58 - Add a negative-path test that triggers and asserts this exception: `spk::LayoutBufferObject::Attribute has an unsupported component type`.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Lines 80-92 - Add a test that reaches this return path: `return *this;`.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Lines 214-216 - Add a negative-path test that triggers and asserts this exception: `spk::LayoutBufferObject requires at least one attribute before vertex upload`.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Lines 218-222 - Add a negative-path test that triggers this exception block: `throw std::runtime_error(`.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 16 (cols 11-17) - Add branch coverage for `p_type`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 23 (cols 3-24) - Add branch coverage for `case Type::Vector2Int`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 24 (cols 3-25) - Add branch coverage for `case Type::Vector2UInt`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 27 (cols 3-24) - Add branch coverage for `case Type::Vector3Int`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 28 (cols 3-25) - Add branch coverage for `case Type::Vector3UInt`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 40 (cols 11-17) - Add branch coverage for `p_type`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 81 (cols 7-23) - Add branch coverage for `this != &p_other`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 192 (cols 27-38) - Add branch coverage for `p_size != 0`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 196 (cols 7-23) - Add branch coverage for `_vertexSize != 0`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 208 (cols 18-34) - Add branch coverage for `_vertexSize == 0`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 213 (cols 7-23) - Add branch coverage for `_vertexSize == 0`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 213 (cols 27-38) - Add branch coverage for `p_size != 0`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 217 (cols 7-23) - Add branch coverage for `_vertexSize != 0`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 217 (cols 27-52) - Add branch coverage for `p_size % _vertexSize != 0`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 224 (cols 7-18) - Add branch coverage for `p_size != 0`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp` - Line 228 (cols 18-34) - Add branch coverage for `_vertexSize == 0`; missing true outcome.

### srcs/structures/graphics/opengl/spk_opengl_object.cpp

Coverage: lines 82.61% (57/69, missing 12); branches 50.00% (10/20, missing 10); functions 92.31% (12/13, missing 1).

- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Lines 48-50 - Add a test that reaches this return path: `return spk::RenderContext::fromId(p_contextId) != nullptr;`.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Lines 87-89 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Lines 97-101 - Add a test that reaches this uncovered block: `spk::RenderContext *owner = spk::RenderContext::fromId(p_object->contextId());`.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 103 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 10 (cols 7-25) - Add branch coverage for `context != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 19 (cols 10-28) - Add branch coverage for `context != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 60 (cols 7-25) - Add branch coverage for `current != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 69 (cols 7-25) - Add branch coverage for `current != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 78 (cols 7-25) - Add branch coverage for `current != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 86 (cols 7-26) - Add branch coverage for `p_object == nullptr`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 92 (cols 7-25) - Add branch coverage for `current != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 92 (cols 29-67) - Add branch coverage for `current->id() == p_object->contextId`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_object.cpp` - Line 98 (cols 7-23) - Add branch coverage for `owner != nullptr`; missing true outcome and false outcome.

### srcs/structures/graphics/opengl/spk_opengl_program.cpp

Coverage: lines 85.14% (149/175, missing 26); branches 47.22% (17/36, missing 19); functions 95.83% (23/24, missing 1).

- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 16-18 - Add a negative-path test that triggers and asserts this exception: `spk::Program requires a current spk::RenderContext to resolve its compiled program`.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 43-50 - Add a negative-path test that triggers this exception block: `throw std::runtime_error("spk::OpenGL::Program link failed: " + log);`.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 108-113 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 115 - Add a test that reaches this uncovered block: `glUseProgram(0);`.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 117-121 - Add a test that reaches this uncovered block: `if (context != nullptr)`.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Lines 224-226 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 15 (cols 7-25) - Add branch coverage for `context == nullptr`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 42 (cols 8-25) - Add branch coverage for `status != GL_TRUE`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 57 (cols 8-16) - Add branch coverage for `_id != 0`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 57 (cols 20-49) - Add branch coverage for `_ownsCurrentContext() == true`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 94 (cols 8-26) - Add branch coverage for `context != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 101 (cols 8-26) - Add branch coverage for `context != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 110 (cols 8-26) - Add branch coverage for `context != nullptr`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 110 (cols 30-71) - Add branch coverage for `context->isProgramActive(nullptr) == true`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 117 (cols 8-26) - Add branch coverage for `context != nullptr`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 176 (cols 10-27) - Add branch coverage for `object != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 176 (cols 31-61) - Add branch coverage for `object->version() == version`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 211 (cols 10-28) - Add branch coverage for `context != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 211 (cols 32-56) - Add branch coverage for `hasGpu(*context) == true`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 223 (cols 7-25) - Add branch coverage for `context != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 223 (cols 29-70) - Add branch coverage for `context->isProgramActive(nullptr) == true`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_program.cpp` - Line 230 (cols 7-25) - Add branch coverage for `context != nullptr`; missing false outcome.

### srcs/structures/graphics/opengl/spk_opengl_render_context.cpp

Coverage: lines 78.21% (219/280, missing 61); branches 69.15% (65/94, missing 29); functions 93.75% (30/32, missing 2).

- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 46-48 - Add a negative-path test that triggers this exception block: `spk::throwLastError("ChoosePixelFormat");`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 51-53 - Add a negative-path test that triggers this exception block: `spk::throwLastError("SetPixelFormat");`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 85-87 - Add a negative-path test that triggers and asserts this exception: `spk::RenderContext requires a valid surface state`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 89-91 - Add a negative-path test that triggers and asserts this exception: `spk::RenderContext requires a valid WinAPI frame window`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 93-95 - Add a negative-path test that triggers this exception block: `spk::throwLastError("GetDC");`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 101-103 - Add a negative-path test that triggers this exception block: `spk::throwLastError("wglCreateContext");`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 110-114 - Add a negative-path test that triggers this exception block: `throw std::runtime_error(`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 166-169 - Add a test that reaches this uncovered block: `wglMakeCurrent(previousDeviceContext, previousRenderContext);`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 181-183 - Add a test that reaches this uncovered block: `s_current = nullptr;`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 198-202 - Add a test that reaches this return path: `return it != contextRegistry().end() ? it->second : nullptr;`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 215-219 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 221-223 - Add a state-changing test that executes this block: `_releaseQueue.push_back(std::move(p_object));`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 270-272 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 300-302 - Add a test that reaches this uncovered block: `_bindingCache.uniformBuffers[i] = nullptr;`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 347 - Add a test that reaches the `wglMakeCurrent(...) == FALSE` context-lost recovery block.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 355-360 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 362-363 - Add a negative-path test that triggers this exception block: `spk::throwLastError("wglMakeCurrent");`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Lines 410-412 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 45 (cols 7-23) - Add branch coverage for `pixelFormat == 0`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 50 (cols 7-73) - Add branch coverage for `SetPixelFormat(p_deviceContext, pixelFormat, &descriptor) == FALSE`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 84 (cols 7-31) - Add branch coverage for `_surfaceState == nullptr`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 88 (cols 7-31) - Add branch coverage for `_windowHandle == nullptr`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 92 (cols 7-32) - Add branch coverage for `_deviceContext == nullptr`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 100 (cols 7-32) - Add branch coverage for `_renderContext == nullptr`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 109 (cols 7-28) - Add branch coverage for `glewResult != GLEW_OK`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 165 (cols 44-83) - Add branch coverage for `previousRenderContext != _renderContext`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 168 (cols 17-40) - Add branch coverage for `previousCurrent != this`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 180 (cols 7-24) - Add branch coverage for `s_current == this`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 185 (cols 35-60) - Add branch coverage for `_deviceContext != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 201 (cols 10-39) - Add branch coverage for `it != contextRegistry().end`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 216 (cols 7-26) - Add branch coverage for `p_object == nullptr`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 260 (cols 10-68) - Add branch coverage for `p_bindingPoint < BindingCache::TrackedUniformBindingPoints`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 262 (cols 7-71) - Add branch coverage for `_bindingCache.uniformBuffers[p_bindingPoint].value() == p_buffer`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 269 (cols 7-66) - Add branch coverage for `p_bindingPoint >= BindingCache::TrackedUniformBindingPoints`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 299 (cols 5-57) - Add branch coverage for `_bindingCache.uniformBuffers[i].value() == &p_buffer`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 320 (cols 7-31) - Add branch coverage for `_surfaceState != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 329 (cols 7-31) - Add branch coverage for `_windowHandle != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 330 (cols 7-32) - Add branch coverage for `_deviceContext != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 331 (cols 7-32) - Add branch coverage for `_renderContext != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 346 (cols 7-62) - Add branch coverage for `wglMakeCurrent(_deviceContext, _renderContext) == FALSE`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 355 (cols 8-40) - Add branch coverage for `IsWindow(_windowHandle) == FALSE`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 391 (cols 7-30) - Add branch coverage for `swapInterval != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_render_context.cpp` - Line 409 (cols 7-25) - Add branch coverage for `isValid() == false`; missing true outcome.

### srcs/structures/graphics/opengl/spk_opengl_texture.cpp

Coverage: lines 81.45% (101/124, missing 23); branches 66.13% (41/62, missing 21); functions 83.33% (5/6, missing 1).

- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Lines 8-24 - Add a switch/enum test case that reaches this branch: `case spk::Texture::Format::GreyLevel:`.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 11 (cols 4-40) - Add branch coverage for `case spk::Texture::Format::GreyLevel`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 13 (cols 4-42) - Add branch coverage for `case spk::Texture::Format::DualChannel`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 15 (cols 4-34) - Add branch coverage for `case spk::Texture::Format::RGB`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 16 (cols 4-34) - Add branch coverage for `case spk::Texture::Format::BGR`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 18 (cols 4-35) - Add branch coverage for `case spk::Texture::Format::RGBA`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 19 (cols 4-35) - Add branch coverage for `case spk::Texture::Format::BGRA`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 21 (cols 4-11) - Add branch coverage for `default`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_texture.cpp` - Line 139 (cols 20-49) - Add branch coverage for `_ownsCurrentContext() == true`; missing false outcome.

### srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp

Coverage: lines 80.68% (142/176, missing 34); branches 56.90% (33/58, missing 25); functions 100.00% (13/13, missing 0).

- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 57-59 - Add a negative-path test that triggers and asserts this exception: `spk::VertexArrayObject contains a null vertex buffer binding`.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 138-141 - Add a loop/control-flow test that reaches this block: `break;`.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 144-146 - Add a test that reaches this uncovered block: `childrenClean = false;`.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 149 - Add a test that reaches the `childrenClean == false` VAO child-refresh block.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 152-168 - Add a test that reaches this uncovered block: `if (p_context.isVertexArrayActive(nullptr) == false)`.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 172-175 - Add a test that reaches this uncovered block: `glBindVertexArray(vertexArray.id());`.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Lines 182-184 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 20 (cols 8-33) - Add branch coverage for `binding.buffer != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 35 (cols 7-21) - Add branch coverage for `ctx != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 35 (cols 25-62) - Add branch coverage for `ctx->supportsOpenGLCommands() == true`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 48 (cols 9-66) - Add branch coverage for `p_context.isVertexArrayActive(vertexArray.get()) == false`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 56 (cols 10-35) - Add branch coverage for `binding.buffer == nullptr`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 69 (cols 7-35) - Add branch coverage for `attribute.normalized == true`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 86 (cols 10-27) - Add branch coverage for `object != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 86 (cols 31-71) - Add branch coverage for `object->version() == _effectiveVersion`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 137 (cols 8-33) - Add branch coverage for `binding.buffer != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 137 (cols 37-79) - Add branch coverage for `binding.buffer->hasGpu(p_context) == false`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 143 (cols 7-28) - Add branch coverage for `childrenClean == true`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 143 (cols 59-99) - Add branch coverage for `_indexBuffer->hasGpu(p_context) == false`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 148 (cols 7-29) - Add branch coverage for `childrenClean == false`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 152 (cols 8-55) - Add branch coverage for `p_context.isVertexArrayActive(nullptr) == false`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 157 (cols 44-45) - Add branch coverage for `for (const VertexBufferBinding &binding : _vertexBufferBindings)`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 159 (cols 9-34) - Add branch coverage for `binding.buffer != nullptr`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 164 (cols 8-31) - Add branch coverage for `_indexBuffer != nullptr`; missing true outcome and false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 171 (cols 7-59) - Add branch coverage for `p_context.isVertexArrayActive(&vertexArray) == false`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 181 (cols 7-25) - Add branch coverage for `context != nullptr`; missing false outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 181 (cols 29-74) - Add branch coverage for `context->isVertexArrayActive(nullptr) == true`; missing true outcome.
- `srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp` - Line 188 (cols 7-25) - Add branch coverage for `context != nullptr`; missing false outcome.

### srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp

Coverage: lines 76.92% (20/26, missing 6); branches 50.00% (2/4, missing 2); functions 100.00% (3/3, missing 0).

- `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Lines 34-36 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Lines 40-42 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Line 33 (cols 7-23) - Add branch coverage for `_mesh == nullptr`; missing true outcome.
- `srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp` - Line 39 (cols 7-37) - Add branch coverage for `layoutBuffer.indexCount() == 0`; missing true outcome.

### srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp

Coverage: lines 100.00% (57/57, missing 0); branches 85.71% (12/14, missing 2); functions 100.00% (4/4, missing 0).

- `srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp` - Line 26 (cols 29-47) - Add branch coverage for `p_cornerSize.y < 0`; missing true outcome.
- `srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp` - Line 32 (cols 4-74) - Add branch coverage for `static_cast<std::uint32_t>(p_cornerSize.y) > p_screenRect.height() / 2`; missing true outcome.

### srcs/structures/graphics/rendering/command/spk_text_render_command.cpp

Coverage: lines 94.74% (108/114, missing 6); branches 81.25% (26/32, missing 6); functions 100.00% (7/7, missing 0).

- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Lines 81-83 - Add a negative-path test that triggers and asserts this exception: `TextRenderCommand font cannot be null`.
- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Lines 87-89 - Add a negative-path test that triggers and asserts this exception: `TextRenderCommand font returned a null atlas`.
- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 33 (cols 29-46) - Add branch coverage for `glyph.size.y != 0`; missing false outcome.
- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 80 (cols 7-23) - Add branch coverage for `_font == nullptr`; missing true outcome.
- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 86 (cols 7-24) - Add branch coverage for `_atlas == nullptr`; missing true outcome.
- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 103 (cols 11-31) - Add branch coverage for `_horizontalAlignment`; missing false outcome.
- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 117 (cols 11-29) - Add branch coverage for `_verticalAlignment`; missing false outcome.
- `srcs/structures/graphics/rendering/command/spk_text_render_command.cpp` - Line 165 (cols 39-62) - Add branch coverage for `_fontCommand == nullptr`; missing true outcome.

### srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp

Coverage: lines 85.00% (17/20, missing 3); branches 75.00% (3/4, missing 1); functions 100.00% (2/2, missing 0).

- `srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp` - Lines 27-29 - Add a negative-path test that triggers and asserts this exception: `spk::ViewportCommand::execute() - render context has no surface state`.
- `srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp` - Line 26 (cols 7-30) - Add branch coverage for `surfaceState == nullptr`; missing true outcome.

### srcs/structures/graphics/spk_gpu_data_buffer_center.cpp

Coverage: lines 93.48% (43/46, missing 3); branches 81.25% (13/16, missing 3); functions 100.00% (6/6, missing 0).

- `srcs/structures/graphics/spk_gpu_data_buffer_center.cpp` - Line 41 (cols 28-46) - Add branch coverage for `*buffer == nullptr`; missing true outcome.
- `srcs/structures/graphics/spk_gpu_data_buffer_center.cpp` - Line 58 (cols 28-46) - Add branch coverage for `*buffer == nullptr`; missing true outcome.

### srcs/structures/graphics/spk_uniform.cpp

Coverage: lines 93.48% (43/46, missing 3); branches 83.33% (5/6, missing 1); functions 100.00% (3/3, missing 0).

- `srcs/structures/graphics/spk_uniform.cpp` - Lines 23-25 - Add a negative-path test that triggers and asserts this exception: `spk::UniformBase: no current render context`.
- `srcs/structures/graphics/spk_uniform.cpp` - Line 22 (cols 7-21) - Add branch coverage for `raw == nullptr`; missing true outcome.

### srcs/structures/graphics/texture/spk_font.cpp

Coverage: lines 86.16% (249/289, missing 40); branches 77.63% (59/76, missing 17); functions 93.33% (28/30, missing 2).

- `srcs/structures/graphics/texture/spk_font.cpp` - Lines 140-142 - Add a test that reaches this uncovered block: `_resizeData(_resource->atlasSize * 2);`.
- `srcs/structures/graphics/texture/spk_font.cpp` - Line 138 (cols 10-107) - Add branch coverage for `p_glyphPosition.x + static_cast<int>(p_glyphSize.x) >= static_cast<int>(_resource->atlasSize.x`; missing true outcome.
- `srcs/structures/graphics/texture/spk_font.cpp` - Line 139 (cols 7-104) - Add branch coverage for `p_glyphPosition.y + static_cast<int>(p_glyphSize.y) >= static_cast<int>(_resource->atlasSize.y`; missing true outcome.
- `srcs/structures/graphics/texture/spk_font.cpp` - Line 304 (cols 8-29) - Add branch coverage for `atlasEntry != nullptr`; missing false outcome.
- `srcs/structures/graphics/texture/spk_font.cpp` - Line 392 (cols 31-52) - Add branch coverage for `it->second == nullptr`; missing true outcome.

### srcs/structures/graphics/texture/spk_font_true_type.cpp

Coverage: lines 91.36% (148/162, missing 14); branches 81.82% (36/44, missing 8); functions 100.00% (7/7, missing 0).

- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Lines 94-97 - Add a test that reaches this uncovered block: `resource.currentQuadrant = Quadrant::DownRight;`.
- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Lines 100-107 - Add a switch/enum test case that reaches this branch: `case Quadrant::DownRight:`.
- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 25 (cols 9-46) - Add branch coverage for `rendered.contains(codepoint) == false`; missing false outcome.
- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 67 (cols 8-78) - Add branch coverage for `resource.nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y`; missing false outcome.
- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 73 (cols 11-35) - Add branch coverage for `resource.currentQuadrant`; missing false outcome.
- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 93 (cols 8-27) - Add branch coverage for `_overflowQuadrant`; missing true outcome.
- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 100 (cols 3-27) - Add branch coverage for `case Quadrant::DownRight`; missing true outcome.
- `srcs/structures/graphics/texture/spk_font_true_type.cpp` - Line 101 (cols 8-27) - Add branch coverage for `_overflowQuadrant`; missing true outcome and false outcome.

### srcs/structures/graphics/texture/spk_image.cpp

Coverage: lines 96.77% (60/62, missing 2); branches 92.86% (13/14, missing 1); functions 100.00% (5/5, missing 0).

- `srcs/structures/graphics/texture/spk_image.cpp` - Lines 85-86 - Add a switch/enum test case that reaches this branch: `default:`.
- `srcs/structures/graphics/texture/spk_image.cpp` - Line 85 (cols 3-10) - Add branch coverage for `default`; missing true outcome.

### srcs/structures/graphics/texture/spk_texture.cpp

Coverage: lines 91.15% (278/305, missing 27); branches 72.37% (55/76, missing 21); functions 100.00% (40/40, missing 0).

- `srcs/structures/graphics/texture/spk_texture.cpp` - Lines 43-45 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/texture/spk_texture.cpp` - Lines 133-135 - Add a test that reaches this return path: `return;`.
- `srcs/structures/graphics/texture/spk_texture.cpp` - Line 42 (cols 7-24) - Add branch coverage for `p_id == InvalidID`; missing true outcome.
- `srcs/structures/graphics/texture/spk_texture.cpp` - Line 114 (cols 10-37) - Add branch coverage for `_state->resource != nullptr`; missing false outcome.
- `srcs/structures/graphics/texture/spk_texture.cpp` - Line 127 (cols 10-37) - Add branch coverage for `_state->resource != nullptr`; missing false outcome.
- `srcs/structures/graphics/texture/spk_texture.cpp` - Line 132 (cols 7-28) - Add branch coverage for `p_resource == nullptr`; missing true outcome.
- `srcs/structures/graphics/texture/spk_texture.cpp` - Line 141 (cols 41-68) - Add branch coverage for `_state->resource != nullptr`; missing false outcome.
- `srcs/structures/graphics/texture/spk_texture.cpp` - Line 152 (cols 25-62) - Add branch coverage for `ctx->supportsOpenGLCommands() == true`; missing false outcome.

### srcs/structures/system/device/window/spk_frame.cpp

Coverage: lines 93.18% (41/44, missing 3); branches 83.33% (5/6, missing 1); functions 90.91% (10/11, missing 1).

- `srcs/structures/system/device/window/spk_frame.cpp` - Lines 22-24 - Add a test that reaches this uncovered block: `(void)p_name;`.
- `srcs/structures/system/device/window/spk_frame.cpp` - Line 48 (cols 7-31) - Add branch coverage for `_surfaceState != nullptr`; missing false outcome.

### srcs/structures/system/device/window/spk_window.cpp

Coverage: lines 88.57% (310/350, missing 40); branches 79.81% (83/104, missing 21); functions 90.91% (40/44, missing 4).

- `srcs/structures/system/device/window/spk_window.cpp` - Lines 15-17 - Add a negative-path test that triggers and asserts this exception: `spk::Window requires a valid spk::PlatformRuntime to create its frame`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 22-24 - Add a negative-path test that triggers and asserts this exception: `spk::Window failed to create its frame`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 151-156 - Add a switch/enum test case that reaches this branch: `case PlatformActionType::SetCursor:`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 187-189 - Add a test that reaches the false-return path: `return false;`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 202-205 - Add a test that reaches this uncovered block: `_appliedCursorShape = requestedShape;`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 304-306 - Add a test that reaches this return path: `return _host;`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 314-316 - Add a test that reaches this return path: `return _mouseModule.mouse();`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 324-326 - Add a test that reaches this return path: `return _keyboardModule.keyboard();`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 334-336 - Add a test that reaches this return path: `return _rootWidget;`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 341-343 - Add a test that reaches this return path: `return;`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 351-353 - Add a test that reaches this return path: `return;`.
- `srcs/structures/system/device/window/spk_window.cpp` - Lines 361-363 - Add a test that reaches this return path: `return;`.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 14 (cols 8-36) - Add branch coverage for `p_platformRuntime == nullptr`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 21 (cols 8-25) - Add branch coverage for `result == nullptr`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 72 (cols 12-23) - Add branch coverage for `action.type`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 131 (cols 11-24) - Add branch coverage for `p_action.type`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 140 (cols 8-42) - Add branch coverage for `p_action.title.has_value() == true`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 146 (cols 8-41) - Add branch coverage for `p_action.rect.has_value() == true`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 151 (cols 3-37) - Add branch coverage for `case PlatformActionType::SetCursor`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 152 (cols 8-48) - Add branch coverage for `p_action.cursorShape.has_value() == true`; missing true outcome and false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 176 (cols 7-57) - Add branch coverage for `_platformResourcesReleased.exchange(true) == false`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 186 (cols 9-33) - Add branch coverage for `_isClosed.load() == true`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 201 (cols 7-44) - Add branch coverage for `requestedShape != _appliedCursorShape`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 228 (cols 7-55) - Add branch coverage for `_renderResourcesReleased.exchange(true) == false`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 236 (cols 11-24) - Add branch coverage for `p_action.type`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 239 (cols 8-41) - Add branch coverage for `p_action.rect.has_value() == true`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 245 (cols 8-49) - Add branch coverage for `p_action.vSyncEnabled.has_value() == true`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 291 (cols 53-85) - Add branch coverage for `_host.isPlatformThread() == true`; missing false outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 340 (cols 7-31) - Add branch coverage for `_isClosed.load() == true`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 350 (cols 7-31) - Add branch coverage for `_isClosed.load() == true`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 360 (cols 7-31) - Add branch coverage for `_isClosed.load() == true`; missing true outcome.
- `srcs/structures/system/device/window/spk_window.cpp` - Line 452 (cols 4-45) - Add branch coverage for `_platformResourcesReleased.load() == true`; missing false outcome.

### srcs/structures/system/device/window/spk_window_handle.cpp

Coverage: lines 86.89% (53/61, missing 8); branches 85.71% (12/14, missing 2); functions 90.00% (9/10, missing 1).

- `srcs/structures/system/device/window/spk_window_handle.cpp` - Lines 75-76 - Add a test that reaches this uncovered block: `std::shared_ptr<spk::Window> window = _window.lock();`.
- `srcs/structures/system/device/window/spk_window_handle.cpp` - Lines 78-81 - Add a negative-path test that triggers and asserts this exception: `WindowHandle::rect : can't access an expired window`.
- `srcs/structures/system/device/window/spk_window_handle.cpp` - Lines 83-84 - Add a test that reaches this return path: `return window->host().rect();`.
- `srcs/structures/system/device/window/spk_window_handle.cpp` - Line 78 (cols 7-24) - Add branch coverage for `window == nullptr`; missing true outcome and false outcome.

### srcs/structures/system/device/window/spk_window_host.cpp

Coverage: lines 89.45% (212/237, missing 25); branches 81.11% (73/90, missing 17); functions 96.00% (24/25, missing 1).

- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 58-62 - Add a negative-path test that triggers this exception block: `throw std::runtime_error(`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 85-87 - Add a negative-path test that triggers and asserts this exception: `spk::WindowHost can't create a render context after its frame has been released`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 154-155 - Add a test that reaches this uncovered block: `_bindOrValidatePlatformThread(__FUNCTION__);`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 157-160 - Add a negative-path test that triggers and asserts this exception: `spk::WindowHost::setCursor called after its frame has been released`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 162-163 - Add a test that reaches this uncovered block: `_frame->setCursor(p_name);`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 275-277 - Add a negative-path test that triggers and asserts this exception: `spk::WindowHost::present called without an initialized render context`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 291-293 - Add a test that reaches this return path: `return;`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Lines 296-298 - Add a test that reaches this return path: `return;`.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 32 (cols 28-61) - Add branch coverage for `_frame->surfaceState() != nullptr`; missing false outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 57 (cols 7-49) - Add branch coverage for `_renderThreadID.value() != currentThreadID`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 84 (cols 7-24) - Add branch coverage for `_frame == nullptr`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 90 (cols 7-30) - Add branch coverage for `surfaceState == nullptr`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 132 (cols 47-72) - Add branch coverage for `_renderContext == nullptr`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 132 (cols 76-110) - Add branch coverage for `_renderContext->isValid() == false`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 157 (cols 7-24) - Add branch coverage for `_frame == nullptr`; missing true outcome and false outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 194 (cols 7-24) - Add branch coverage for `_frame != nullptr`; missing false outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 194 (cols 7-61) - Add branch coverage for `_frame != nullptr && _frame->surfaceState() != nullptr`; missing false outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 194 (cols 28-61) - Add branch coverage for `_frame->surfaceState() != nullptr`; missing false outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 205 (cols 47-72) - Add branch coverage for `_renderContext == nullptr`; missing false outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 261 (cols 48-73) - Add branch coverage for `_renderContext == nullptr`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 261 (cols 77-111) - Add branch coverage for `_renderContext->isValid() == false`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 274 (cols 7-32) - Add branch coverage for `_renderContext == nullptr`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 290 (cols 7-44) - Add branch coverage for `_ensureRenderContextLocked() == false`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_host.cpp` - Line 295 (cols 7-41) - Add branch coverage for `_renderContext->isValid() == false`; missing true outcome.

### srcs/structures/system/device/window/spk_window_registry.cpp

Coverage: lines 90.91% (70/77, missing 7); branches 81.25% (13/16, missing 3); functions 100.00% (7/7, missing 0).

- `srcs/structures/system/device/window/spk_window_registry.cpp` - Lines 19-21 - Add a negative-path test that triggers this exception block: `throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");`.
- `srcs/structures/system/device/window/spk_window_registry.cpp` - Lines 38-40 - Add a negative-path test that triggers this exception block: `throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] doesn't exist");`.
- `srcs/structures/system/device/window/spk_window_registry.cpp` - Line 106 - Add a test that reaches this uncovered block: `windowToClose->requestClosure();`.
- `srcs/structures/system/device/window/spk_window_registry.cpp` - Line 18 (cols 7-38) - Add branch coverage for `_windows.contains(p_id) == true`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_registry.cpp` - Line 37 (cols 7-33) - Add branch coverage for `iterator == _windows.end`; missing true outcome.
- `srcs/structures/system/device/window/spk_window_registry.cpp` - Line 79 (cols 8-42) - Add branch coverage for `iterator->second.window == nullptr`; missing true outcome.

### srcs/structures/system/time/spk_chronometer.cpp

Coverage: lines 93.90% (77/82, missing 5); branches 88.46% (23/26, missing 3); functions 100.00% (12/12, missing 0).

- `srcs/structures/system/time/spk_chronometer.cpp` - Lines 73-75 - Add a test that reaches this return path: `return Duration();`.
- `srcs/structures/system/time/spk_chronometer.cpp` - Line 72 (cols 7-31) - Add branch coverage for `_state != State::Running`; missing true outcome.

### srcs/structures/system/win32/spk_winapi_class.cpp

Coverage: lines 95.89% (70/73, missing 3); branches 75.00% (18/24, missing 6); functions 100.00% (6/6, missing 0).

- `srcs/structures/system/win32/spk_winapi_class.cpp` - Lines 40-42 - Add a negative-path test that triggers this exception block: `throwLastError("RegisterClassExW");`.
- `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 39 (cols 8-47) - Add branch coverage for `errorCode != ERROR_CLASS_ALREADY_EXISTS`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 62 (cols 32-52) - Add branch coverage for `_instance != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 62 (cols 56-84) - Add branch coverage for `_nativeName.empty() == false`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 75 (cols 7-28) - Add branch coverage for `_isRegistered == true`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 75 (cols 32-52) - Add branch coverage for `_instance != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_class.cpp` - Line 75 (cols 56-84) - Add branch coverage for `_nativeName.empty() == false`; missing false outcome.

### srcs/structures/system/win32/spk_winapi_cursor.cpp

Coverage: lines 97.78% (132/135, missing 3); branches 85.00% (17/20, missing 3); functions 100.00% (24/24, missing 0).

- `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Lines 142-144 - Add a negative-path test that triggers this exception block: `throw std::runtime_error("spk::Cursor::registerCursor: CopyCursor failed for '" + p_name + "'");`.
- `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Line 63 (cols 30-48) - Add branch coverage for `_handle != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Line 76 (cols 30-48) - Add branch coverage for `_handle != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_cursor.cpp` - Line 141 (cols 7-22) - Add branch coverage for `copy == nullptr`; missing true outcome.

### srcs/structures/system/win32/spk_winapi_frame.cpp

Coverage: lines 95.43% (209/219, missing 10); branches 90.54% (67/74, missing 7); functions 94.44% (17/18, missing 1).

- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 63 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 72 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 99 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 107 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 114 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Lines 221-225 - Add a test that reaches this uncovered block: `spk::Cursor cursor = spk::Cursor::get(p_name);`.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 20 (cols 3-24) - Add branch coverage for `case WM_RBUTTONDBLCLK`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 24 (cols 3-24) - Add branch coverage for `case WM_MBUTTONDBLCLK`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 136 (cols 3-24) - Add branch coverage for `case WM_RBUTTONDBLCLK`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 137 (cols 3-24) - Add branch coverage for `case WM_MBUTTONDBLCLK`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 154 (cols 3-19) - Add branch coverage for `case WM_SYSKEYUP`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 179 (cols 7-38) - Add branch coverage for `TrackMouseEvent(&event) == TRUE`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_frame.cpp` - Line 234 (cols 7-32) - Add branch coverage for `_window.isValid() == true`; missing false outcome.

### srcs/structures/system/win32/spk_winapi_window.cpp

Coverage: lines 90.50% (181/200, missing 19); branches 80.36% (45/56, missing 11); functions 100.00% (18/18, missing 0).

- `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 38-41 - Add a test that reaches this uncovered block: `result = DefWindowProcW(_handle, p_message, p_wParam, p_lParam);`.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 73-75 - Add a negative-path test that triggers this exception block: `throwLastError("AdjustWindowRectEx");`.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 92-94 - Add a negative-path test that triggers this exception block: `throwLastError("CreateWindowExW");`.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 168-170 - Add a negative-path test that triggers this exception block: `throwLastError("SetWindowTextW");`.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 189-191 - Add a negative-path test that triggers this exception block: `throwLastError("AdjustWindowRectEx");`.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 201-203 - Add a negative-path test that triggers this exception block: `throwLastError("SetWindowPos");`.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 34 (cols 7-28) - Add branch coverage for `_procedure != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 72 (cols 7-80) - Add branch coverage for `AdjustWindowRectEx(&nativeRect, p_style, FALSE, p_extendedStyle) == FALSE`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 91 (cols 7-25) - Add branch coverage for `_handle == nullptr`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 103 (cols 7-25) - Add branch coverage for `_handle != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 127 (cols 7-25) - Add branch coverage for `_handle != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 167 (cols 7-92) - Add branch coverage for `_handle != nullptr && SetWindowTextW(_handle, toWideString(p_title).c_str()) == FALSE`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 167 (cols 29-92) - Add branch coverage for `SetWindowTextW(_handle, toWideString(p_title).c_str()) == FALSE`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 188 (cols 7-76) - Add branch coverage for `AdjustWindowRectEx(&nativeRect, style, FALSE, extendedStyle) == FALSE`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Lines 193-200 (cols 7-44) - Add branch coverage for `if (SetWindowPos( _handle, nullptr, nativeRect.left,`; missing true outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 208 (cols 7-25) - Add branch coverage for `_handle != nullptr`; missing false outcome.
- `srcs/structures/system/win32/spk_winapi_window.cpp` - Line 272 (cols 32-57) - Add branch coverage for `IsWindow(_handle) == TRUE`; missing false outcome.

### srcs/structures/widget/spk_action_bar.cpp

Coverage: lines 92.93% (263/283, missing 20); branches 67.50% (27/40, missing 13); functions 100.00% (29/29, missing 0).

- `srcs/structures/widget/spk_action_bar.cpp` - Lines 73-75 - Add a test that reaches this return path: `return builder.build();`.
- `srcs/structures/widget/spk_action_bar.cpp` - Lines 118-120 - Add a test that reaches this uncovered block: `_spriteSheet = std::move(p_spriteSheet);`.
- `srcs/structures/widget/spk_action_bar.cpp` - Lines 164-173 - Add a test that reaches this uncovered block: `_controlHeight = p_controlHeight;`.
- `srcs/structures/widget/spk_action_bar.cpp` - Lines 250-253 - Add a test that reaches this uncovered block: `_onGeometryChange();`.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 72 (cols 7-30) - Add branch coverage for `_spriteSheet == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 72 (cols 34-60) - Add branch coverage for `geometry().empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 86 (cols 7-22) - Add branch coverage for `middleWidth > 0`; missing false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 113 (cols 7-58) - Add branch coverage for `p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 1}`; missing false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 159 (cols 7-40) - Add branch coverage for `_controlHeight == p_controlHeight`; missing false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 165 (cols 28-29) - Add branch coverage for `for (const auto &element : _elements)`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 167 (cols 76-93) - Add branch coverage for `button != nullptr`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 196 (cols 7-70) - Add branch coverage for `absoluteGeometry().contains(p_event.device().position) == false`; missing false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 246 (cols 7-21) - Add branch coverage for `bar != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 273 (cols 5-63) - Add branch coverage for `_height > static_cast<unsigned int>(BarContentInset * 2`; missing false outcome.
- `srcs/structures/widget/spk_action_bar.cpp` - Line 333 (cols 9-30) - Add branch coverage for `wasActivated == false`; missing false outcome.

### srcs/structures/widget/spk_animation_label.cpp

Coverage: lines 96.04% (97/101, missing 4); branches 76.92% (20/26, missing 6); functions 100.00% (13/13, missing 0).

- `srcs/structures/widget/spk_animation_label.cpp` - Lines 70-73 - Add a test that reaches this uncovered block: `_currentSprite = (_currentSprite + 1) % _spriteSheet->sprites().size();`.
- `srcs/structures/widget/spk_animation_label.cpp` - Line 31 (cols 4-44) - Add branch coverage for `_spriteSheet->sprites().empty() == false`; missing false outcome.
- `srcs/structures/widget/spk_animation_label.cpp` - Line 32 (cols 4-31) - Add branch coverage for `geometry().empty() == false`; missing false outcome.
- `srcs/structures/widget/spk_animation_label.cpp` - Line 52 (cols 34-73) - Add branch coverage for `_spriteSheet->sprites().empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_animation_label.cpp` - Line 62 (cols 7-31) - Add branch coverage for `_rangeEnd >= _rangeStart`; missing false outcome.
- `srcs/structures/widget/spk_animation_label.cpp` - Line 62 (cols 35-77) - Add branch coverage for `_rangeEnd < _spriteSheet->sprites().size`; missing false outcome.
- `srcs/structures/widget/spk_animation_label.cpp` - Line 89 (cols 15-57) - Add branch coverage for `_spriteSheet->sprites().empty() == false`; missing false outcome.

### srcs/structures/widget/spk_checkable_icon_button.cpp

Coverage: lines 89.54% (137/153, missing 16); branches 70.00% (7/10, missing 3); functions 88.46% (23/26, missing 3).

- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 71-72 - Add a test that reaches this uncovered block: `const spk::Vector2UInt uncheckedSize = _uncheckedButton.minimalSize();`.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 74-77 - Add a test that reaches this return path: `return spk::Vector2UInt(`.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 123-125 - Add a test that reaches this uncovered block: `_checkedButton.removeIcon();`.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 169-171 - Add a test that reaches this return path: `return _uncheckedButton;`.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Lines 179-181 - Add a test that reaches this return path: `return _checkedButton;`.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Line 70 - Add a test that calls this currently unexecuted function: `configureMinimalSizeGenerator([this]() {`.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Line 117 (cols 7-32) - Add branch coverage for `uncheckedHadIcon == false`; missing false outcome.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Line 122 (cols 7-30) - Add branch coverage for `checkedHadIcon == false`; missing true outcome.
- `srcs/structures/widget/spk_checkable_icon_button.cpp` - Line 192 (cols 34-53) - Add branch coverage for `callback != nullptr`; missing false outcome.

### srcs/structures/widget/spk_command_panel.cpp

Coverage: lines 94.23% (98/104, missing 6); branches 83.33% (15/18, missing 3); functions 100.00% (13/13, missing 0).

- `srcs/structures/widget/spk_command_panel.cpp` - Lines 88-90 - Add a negative-path test that triggers this exception block: `throw std::runtime_error("Button [" + p_name + "] doesn't exist in the command panel [" + name() + "]");`.
- `srcs/structures/widget/spk_command_panel.cpp` - Lines 98-100 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_command_panel.cpp` - Line 46 (cols 58-112) - Add branch coverage for `_sizePolicy == spk::Layout::SizePolicy::VerticalExtend`; missing true outcome.
- `srcs/structures/widget/spk_command_panel.cpp` - Line 87 (cols 7-41) - Add branch coverage for `_buttons.contains(p_name) == false`; missing true outcome.
- `srcs/structures/widget/spk_command_panel.cpp` - Line 97 (cols 7-27) - Add branch coverage for `it == _buttons.end`; missing true outcome.

### srcs/structures/widget/spk_container_widget.cpp

Coverage: lines 88.00% (44/50, missing 6); branches 70.00% (7/10, missing 3); functions 100.00% (10/10, missing 0).

- `srcs/structures/widget/spk_container_widget.cpp` - Lines 16-18 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_container_widget.cpp` - Lines 53-55 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_container_widget.cpp` - Line 15 (cols 7-26) - Add branch coverage for `_content == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_container_widget.cpp` - Line 30 (cols 7-27) - Add branch coverage for `p_content != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_container_widget.cpp` - Line 52 (cols 7-36) - Add branch coverage for `_contentSize == p_contentSize`; missing true outcome.

### srcs/structures/widget/spk_debug_overlay.cpp

Coverage: lines 100.00% (180/180, missing 0); branches 81.82% (54/66, missing 12); functions 100.00% (19/19, missing 0).

- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 30 (cols 21-39) - Add branch coverage for `_font != nullptr`; missing true outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 48 (cols 9-25) - Add branch coverage for `label != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 64 (cols 9-25) - Add branch coverage for `label == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 64 (cols 29-53) - Add branch coverage for `label->font() == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 70 (cols 24-35) - Add branch coverage for `area.y == 0`; missing true outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 78 (cols 9-37) - Add branch coverage for `glyphSize > _outlineSize * 2`; missing false outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 82 (cols 30-55) - Add branch coverage for `glyphSize > _maxGlyphSize`; missing false outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 174 (cols 9-25) - Add branch coverage for `label != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 193 (cols 9-25) - Add branch coverage for `label != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 230 (cols 32-70) - Add branch coverage for `p_column >= _rows[p_row].labels.size`; missing true outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 239 (cols 32-63) - Add branch coverage for `_rows[0].labels.empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_debug_overlay.cpp` - Line 239 (cols 67-96) - Add branch coverage for `_rows[0].labels[0] == nullptr`; missing true outcome.

### srcs/structures/widget/spk_dynamic_text_label.cpp

Coverage: lines 100.00% (52/52, missing 0); branches 87.50% (7/8, missing 1); functions 100.00% (8/8, missing 0).

- `srcs/structures/widget/spk_dynamic_text_label.cpp` - Line 58 (cols 7-31) - Add branch coverage for `_textProducer != nullptr`; missing false outcome.

### srcs/structures/widget/spk_form_layout.cpp

Coverage: lines 78.45% (233/297, missing 64); branches 58.22% (85/146, missing 61); functions 88.89% (24/27, missing 3).

- `srcs/structures/widget/spk_form_layout.cpp` - Lines 11-13 - Add a test that reaches this return path: `return std::numeric_limits<spk::Vector2UInt::value_type>::max();`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 21-23 - Add a test that reaches this return path: `return static_cast<uint32_t>(std::min(p_value, static_cast<size_t>(unlimitedSize())));`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 43-45 - Add a test that reaches this return path: `return {0U, 0U};`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 56-58 - Add a test that reaches this return path: `return 0U;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 62-65 - Add a switch/enum test case that reaches this branch: `case spk::FormLayout::SizePolicy::Fixed:`.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 72 - Add a test that reaches this return path: `return 0U;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 82-85 - Add a switch/enum test case that reaches this branch: `case spk::FormLayout::SizePolicy::Fixed:`.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 92 - Add a test that reaches this return path: `return 0;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 98-100 - Add a test that reaches the false-return path: `return false;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 109-111 - Add a test that reaches the false-return path: `return false;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 158-160 - Add a test that reaches this uncovered block: `result.labelColumnExpandable = true;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 193-196 - Add a test that reaches this uncovered block: `p_labelExpandable = true;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 205-207 - Add a test that reaches this uncovered block: `std::fill(p_rowExpandable.begin(), p_rowExpandable.end(), true);`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 227-229 - Add a test that reaches this uncovered block: `p_labelWidth += share + ((remainder-- > 0U) ? 1U : 0U);`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 245-247 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 281-283 - Add a test that reaches this uncovered block: `labelAvailableWidth = p_availableSize.x;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 285-287 - Add a test that reaches this uncovered block: `fieldAvailableWidth = p_availableSize.x;`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 323-330 - Add a test that reaches this uncovered block: `else if (labelRenderable == true)`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 345-346 - Add a test that reaches this return path: `return _computeMinimalSize();`.
- `srcs/structures/widget/spk_form_layout.cpp` - Lines 419-421 - Add a test that reaches this return path: `return {0U, 0U};`.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 344 - Add a test that calls this currently unexecuted function: `configureFixedSizeGenerator([this]() {`.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 17 (cols 10-32) - Add branch coverage for `p_element != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 17 (cols 37-67) - Add branch coverage for `p_element->widget() != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 17 (cols 71-101) - Add branch coverage for `p_element->layout() != nullptr`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 42 (cols 7-40) - Add branch coverage for `hasRenderable(p_element) == false`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 55 (cols 7-40) - Add branch coverage for `hasRenderable(p_element) == false`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 60 (cols 11-34) - Add branch coverage for `p_element->sizePolicy`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 62 (cols 3-42) - Add branch coverage for `case spk::FormLayout::SizePolicy::Fixed`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 64 (cols 3-44) - Add branch coverage for `case spk::FormLayout::SizePolicy::Maximum`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 67 (cols 3-53) - Add branch coverage for `case spk::FormLayout::SizePolicy::HorizontalExtend`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 69 (cols 3-51) - Add branch coverage for `case spk::FormLayout::SizePolicy::VerticalExtend`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 80 (cols 11-34) - Add branch coverage for `p_element->sizePolicy`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 82 (cols 3-42) - Add branch coverage for `case spk::FormLayout::SizePolicy::Fixed`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 84 (cols 3-44) - Add branch coverage for `case spk::FormLayout::SizePolicy::Maximum`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 87 (cols 3-53) - Add branch coverage for `case spk::FormLayout::SizePolicy::HorizontalExtend`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 89 (cols 3-51) - Add branch coverage for `case spk::FormLayout::SizePolicy::VerticalExtend`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 97 (cols 7-27) - Add branch coverage for `p_element == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 103 (cols 60-115) - Add branch coverage for `policy == spk::FormLayout::SizePolicy::HorizontalExtend`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 108 (cols 7-27) - Add branch coverage for `p_element == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 113 (cols 60-113) - Add branch coverage for `policy == spk::FormLayout::SizePolicy::VerticalExtend`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 130 (cols 11-40) - Add branch coverage for `p_scan.hasLabelColumn == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 130 (cols 44-73) - Add branch coverage for `p_scan.hasFieldColumn == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 148 (cols 31-54) - Add branch coverage for `labelRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 148 (cols 58-81) - Add branch coverage for `fieldRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 150 (cols 8-31) - Add branch coverage for `labelRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 157 (cols 9-53) - Add branch coverage for `elementHorizontallyExpandable(label) == true`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 157 (cols 57-80) - Add branch coverage for `fieldRenderable == true`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 163 (cols 8-31) - Add branch coverage for `fieldRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 168 (cols 9-32) - Add branch coverage for `labelRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 178 (cols 9-53) - Add branch coverage for `elementHorizontallyExpandable(field) == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 178 (cols 57-80) - Add branch coverage for `labelRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 184 (cols 33-67) - Add branch coverage for `elementVerticallyExpandable(label`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 184 (cols 71-105) - Add branch coverage for `elementVerticallyExpandable(field`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 192 (cols 7-33) - Add branch coverage for `p_labelExpandable == false`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 192 (cols 37-63) - Add branch coverage for `p_fieldExpandable == false`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 204 (cols 7-19) - Add branch coverage for `any == false`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 218 (cols 7-24) - Add branch coverage for `expandables == 0U`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 226 (cols 7-32) - Add branch coverage for `p_labelExpandable == true`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 228 (cols 29-47) - Add branch coverage for `remainder-- > 0U`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 230 (cols 7-32) - Add branch coverage for `p_fieldExpandable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 232 (cols 29-47) - Add branch coverage for `remainder-- > 0U`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 244 (cols 7-18) - Add branch coverage for `count == 0U`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 254 (cols 8-34) - Add branch coverage for `p_rowExpandable[i] == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 280 (cols 8-31) - Add branch coverage for `labelRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 280 (cols 35-59) - Add branch coverage for `fieldRenderable == false`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 284 (cols 13-36) - Add branch coverage for `fieldRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 284 (cols 40-64) - Add branch coverage for `labelRenderable == false`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 290 (cols 5-20) - Add branch coverage for `labelRenderable`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 292 (cols 5-20) - Add branch coverage for `fieldRenderable`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 318 (cols 8-31) - Add branch coverage for `labelRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 318 (cols 35-58) - Add branch coverage for `fieldRenderable == true`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 323 (cols 13-36) - Add branch coverage for `labelRenderable == true`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 327 (cols 13-36) - Add branch coverage for `fieldRenderable == true`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 384 (cols 31-66) - Add branch coverage for `p_geometry.size.x > minTotalWidth`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 392 (cols 48-61) - Add branch coverage for `rowCount > 0U`; missing false outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 418 (cols 7-26) - Add branch coverage for `rowCountValue == 0U`; missing true outcome.
- `srcs/structures/widget/spk_form_layout.cpp` - Line 438 (cols 27-45) - Add branch coverage for `rowCountValue > 0U`; missing false outcome.

### srcs/structures/widget/spk_icon_button.cpp

Coverage: lines 87.76% (43/49, missing 6); branches 50.00% (3/6, missing 3); functions 100.00% (9/9, missing 0).

- `srcs/structures/widget/spk_icon_button.cpp` - Lines 29-31 - Add a negative-path test that triggers and asserts this exception: `IconButton iconset cannot be null`.
- `srcs/structures/widget/spk_icon_button.cpp` - Lines 59-61 - Add a test that reaches this uncovered block: `_iconset = std::move(p_iconset);`.
- `srcs/structures/widget/spk_icon_button.cpp` - Line 28 (cols 7-26) - Add branch coverage for `_iconset == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_icon_button.cpp` - Line 45 (cols 7-43) - Add branch coverage for `p_style.iconSpriteSheet() != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_icon_button.cpp` - Line 54 (cols 7-27) - Add branch coverage for `p_iconset == nullptr`; missing false outcome.

### srcs/structures/widget/spk_image_label.cpp

Coverage: lines 94.44% (51/54, missing 3); branches 80.00% (8/10, missing 2); functions 100.00% (9/9, missing 0).

- `srcs/structures/widget/spk_image_label.cpp` - Lines 67-69 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_image_label.cpp` - Line 30 (cols 30-57) - Add branch coverage for `geometry().empty() == false`; missing false outcome.
- `srcs/structures/widget/spk_image_label.cpp` - Line 66 (cols 7-24) - Add branch coverage for `_depth == p_depth`; missing true outcome.

### srcs/structures/widget/spk_interface_window.cpp

Coverage: lines 89.66% (364/406, missing 42); branches 66.28% (57/86, missing 29); functions 92.59% (50/54, missing 4).

- `srcs/structures/widget/spk_interface_window.cpp` - Lines 122-127 - Add a switch/enum test case that reaches this branch: `case Button::Maximize:`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 139-144 - Add a switch/enum test case that reaches this branch: `case Button::Maximize:`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 155-157 - Add a test that reaches this return path: `return _titleLabel;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 165-167 - Add a test that reaches this return path: `return _minimizeButton;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 175-177 - Add a test that reaches this return path: `return _maximizeButton;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 185-187 - Add a test that reaches this return path: `return _closeButton;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 232 - Add a negative-path test that triggers and asserts this exception: `Unknown interface window event`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 303-305 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 318-322 - Add a test that reaches this uncovered block: `if (absoluteGeometry().contains(p_event.device().position) == true)`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 329-331 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 404-406 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Lines 421-423 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 26 (cols 10-67) - Add branch coverage for `p_left > std::numeric_limits<uint32_t>::max() - p_right`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 82 (cols 4-52) - Add branch coverage for `height > static_cast<unsigned int>(Margin * 2`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 92 (cols 5-34) - Add branch coverage for `_maximizeButton.isActivated`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 93 (cols 5-31) - Add branch coverage for `_closeButton.isActivated`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 117 (cols 11-19) - Add branch coverage for `p_button`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 119 (cols 3-24) - Add branch coverage for `case Button::Minimize`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 122 (cols 3-24) - Add branch coverage for `case Button::Maximize`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 125 (cols 3-21) - Add branch coverage for `case Button::Close`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 134 (cols 11-19) - Add branch coverage for `p_button`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 136 (cols 3-24) - Add branch coverage for `case Button::Minimize`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 139 (cols 3-24) - Add branch coverage for `case Button::Maximize`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 142 (cols 3-21) - Add branch coverage for `case Button::Close`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 222 (cols 11-18) - Add branch coverage for `p_event`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 242 (cols 4-43) - Add branch coverage for `geometry().width() > horizontalMargin`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 289 (cols 7-35) - Add branch coverage for `p_event.isConsumed() == true`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 302 (cols 7-35) - Add branch coverage for `p_event.isConsumed() == true`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 307 (cols 7-42) - Add branch coverage for `p_event->button == spk::Mouse::Left`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 308 (cols 4-91) - Add branch coverage for `_menuBar.titleLabel().viewport().geometry().contains(p_event.device().position) == true`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 318 (cols 7-69) - Add branch coverage for `absoluteGeometry().contains(p_event.device().position) == true`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 328 (cols 7-42) - Add branch coverage for `p_event->button != spk::Mouse::Left`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 328 (cols 46-64) - Add branch coverage for `_isMoving == false`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 403 (cols 7-42) - Add branch coverage for `_contentPadding.has_value() == true`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 403 (cols 46-75) - Add branch coverage for `*_contentPadding == p_padding`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 410 (cols 7-26) - Add branch coverage for `_content != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 420 (cols 7-43) - Add branch coverage for `_contentPadding.has_value() == false`; missing true outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 427 (cols 7-26) - Add branch coverage for `_content != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_interface_window.cpp` - Line 456 (cols 7-26) - Add branch coverage for `_content != nullptr`; missing false outcome.

### srcs/structures/widget/spk_layout.cpp

Coverage: lines 91.30% (168/184, missing 16); branches 71.74% (33/46, missing 13); functions 96.55% (28/29, missing 1).

- `srcs/structures/widget/spk_layout.cpp` - Line 56 - Add a test that reaches this return path: `return {0u, 0u};`.
- `srcs/structures/widget/spk_layout.cpp` - Line 69 - Add a test that reaches this return path: `return {0u, 0u};`.
- `srcs/structures/widget/spk_layout.cpp` - Lines 78-83 - Add a test that reaches this return path: `return _layout->fixedSize();`.
- `srcs/structures/widget/spk_layout.cpp` - Line 95 - Add a test that reaches this return path: `return {0u, 0u};`.
- `srcs/structures/widget/spk_layout.cpp` - Lines 138-141 - Add a test that reaches this return path: `return spk::ResizableElement::fixedSize();`.
- `srcs/structures/widget/spk_layout.cpp` - Lines 219-221 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_layout.cpp` - Line 52 (cols 7-25) - Add branch coverage for `_layout != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 65 (cols 7-25) - Add branch coverage for `_layout != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 74 (cols 7-25) - Add branch coverage for `_widget != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 78 (cols 7-25) - Add branch coverage for `_layout != nullptr`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 91 (cols 7-25) - Add branch coverage for `_layout != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 104 (cols 12-30) - Add branch coverage for `_layout != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 190 (cols 12-41) - Add branch coverage for `p_element->isLayout() == true`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 205 (cols 8-22) - Add branch coverage for `*it != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 205 (cols 26-53) - Add branch coverage for `*it)->widget() == p_widget`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 218 (cols 7-26) - Add branch coverage for `p_layout == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 225 (cols 8-22) - Add branch coverage for `*it != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_layout.cpp` - Line 225 (cols 26-53) - Add branch coverage for `*it)->layout() == p_layout`; missing false outcome.

### srcs/structures/widget/spk_message_box.cpp

Coverage: lines 92.56% (199/215, missing 16); branches 62.50% (5/8, missing 3); functions 87.18% (34/39, missing 5).

- `srcs/structures/widget/spk_message_box.cpp` - Lines 53-55 - Add a test that reaches this return path: `return _layout;`.
- `srcs/structures/widget/spk_message_box.cpp` - Lines 84-85 - Add a test that reaches this uncovered block: `close();`.
- `srcs/structures/widget/spk_message_box.cpp` - Line 200 - Add a test that reaches this uncovered block: `},`.
- `srcs/structures/widget/spk_message_box.cpp` - Line 203 - Add a test that reaches this uncovered block: `});`.
- `srcs/structures/widget/spk_message_box.cpp` - Lines 235-239 - Add a test that reaches this uncovered block: `if (action != nullptr)`.
- `srcs/structures/widget/spk_message_box.cpp` - Line 83 - Add a test that calls this currently unexecuted function: `_closeContract = subscribeTo(spk::IInterfaceWindow::Event::Close, [this]() {`.
- `srcs/structures/widget/spk_message_box.cpp` - Line 199 - Add a test that calls this currently unexecuted function: `configure("Yes", []() {`.
- `srcs/structures/widget/spk_message_box.cpp` - Line 202 - Add a test that calls this currently unexecuted function: `[]() {`.
- `srcs/structures/widget/spk_message_box.cpp` - Line 234 - Add a test that calls this currently unexecuted function: `[action = p_secondAction]() {`.
- `srcs/structures/widget/spk_message_box.cpp` - Line 225 (cols 9-26) - Add branch coverage for `action != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_message_box.cpp` - Line 235 (cols 9-26) - Add branch coverage for `action != nullptr`; missing true outcome and false outcome.

### srcs/structures/widget/spk_panel.cpp

Coverage: lines 96.97% (96/99, missing 3); branches 81.25% (13/16, missing 3); functions 100.00% (17/17, missing 0).

- `srcs/structures/widget/spk_panel.cpp` - Lines 122-124 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_panel.cpp` - Line 85 (cols 7-30) - Add branch coverage for `_spriteSheet != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_panel.cpp` - Line 85 (cols 34-61) - Add branch coverage for `geometry().empty() == false`; missing false outcome.
- `srcs/structures/widget/spk_panel.cpp` - Line 121 (cols 7-34) - Add branch coverage for `_cornerSize == p_cornerSize`; missing true outcome.

### srcs/structures/widget/spk_push_button.cpp

Coverage: lines 92.04% (312/339, missing 27); branches 79.17% (38/48, missing 10); functions 95.65% (44/46, missing 2).

- `srcs/structures/widget/spk_push_button.cpp` - Lines 21-23 - Add a test that reaches this return path: `return {0, 0};`.
- `srcs/structures/widget/spk_push_button.cpp` - Lines 256-258 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_push_button.cpp` - Lines 266-270 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_push_button.cpp` - Lines 272-275 - Add a test that reaches this uncovered block: `_iconSize.reset();`.
- `srcs/structures/widget/spk_push_button.cpp` - Lines 280-282 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_push_button.cpp` - Lines 290-294 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_push_button.cpp` - Lines 296-299 - Add a test that reaches this uncovered block: `_iconPadding.reset();`.
- `srcs/structures/widget/spk_push_button.cpp` - Line 20 (cols 8-35) - Add branch coverage for `p_icon.texture() == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_push_button.cpp` - Line 255 (cols 7-30) - Add branch coverage for `_iconSize == p_iconSize`; missing true outcome.
- `srcs/structures/widget/spk_push_button.cpp` - Line 267 (cols 7-37) - Add branch coverage for `_iconSize.has_value() == false`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_push_button.cpp` - Line 279 (cols 7-39) - Add branch coverage for `_iconPadding.has_value() == true`; missing true outcome.
- `srcs/structures/widget/spk_push_button.cpp` - Line 279 (cols 43-73) - Add branch coverage for `*_iconPadding == p_iconPadding`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_push_button.cpp` - Line 291 (cols 7-40) - Add branch coverage for `_iconPadding.has_value() == false`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_push_button.cpp` - Line 439 (cols 7-42) - Add branch coverage for `p_event->button != spk::Mouse::Left`; missing true outcome.

### srcs/structures/widget/spk_resizable_element.cpp

Coverage: lines 90.57% (48/53, missing 5); branches 0.00% (0/0, missing 0); functions 94.44% (17/18, missing 1).

- `srcs/structures/widget/spk_resizable_element.cpp` - Lines 11-15 - Add a test that reaches this uncovered block: `configureMinimalSizeGenerator(std::move(p_minimalGenerator));`.

### srcs/structures/widget/spk_scalable_widget.cpp

Coverage: lines 90.29% (186/206, missing 20); branches 71.43% (40/56, missing 16); functions 100.00% (14/14, missing 0).

- `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 95-97 - Add a test that reaches this uncovered block: `edges |= Edge::Bottom;`.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 125-127 - Add a test that reaches this uncovered block: `p_mouse.requestCursor("ResizeNS");`.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 174-184 - Add a test that reaches this uncovered block: `compute1DResize(`.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Lines 242-244 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 94 (cols 7-51) - Add branch coverage for `bottomArea.contains(p_mousePosition) == true`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 116 (cols 25-31) - Add branch coverage for `bottom`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 116 (cols 35-40) - Add branch coverage for `right`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 120 (cols 20-25) - Add branch coverage for `right`; missing false outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 120 (cols 31-37) - Add branch coverage for `bottom`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 120 (cols 41-45) - Add branch coverage for `left`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 124 (cols 12-15) - Add branch coverage for `top`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 124 (cols 19-25) - Add branch coverage for `bottom`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 128 (cols 12-16) - Add branch coverage for `left`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 173 (cols 12-46) - Add branch coverage for `_activeEdges & Edge::Bottom) != 0`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 198 (cols 12-45) - Add branch coverage for `_activeEdges & Edge::Right) != 0`; missing false outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 232 (cols 7-32) - Add branch coverage for `newGeometry != geometry`; missing false outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 241 (cols 7-42) - Add branch coverage for `p_event->button != spk::Mouse::Left`; missing true outcome.
- `srcs/structures/widget/spk_scalable_widget.cpp` - Line 257 (cols 7-42) - Add branch coverage for `p_event->button != spk::Mouse::Left`; missing true outcome.

### srcs/structures/widget/spk_screen.cpp

Coverage: lines 100.00% (26/26, missing 0); branches 83.33% (5/6, missing 1); functions 100.00% (4/4, missing 0).

- `srcs/structures/widget/spk_screen.cpp` - Line 9 (cols 36-57) - Add branch coverage for `_activeScreen != this`; missing false outcome.

### srcs/structures/widget/spk_scroll_area.cpp

Coverage: lines 97.74% (130/133, missing 3); branches 82.35% (28/34, missing 6); functions 100.00% (22/22, missing 0).

- `srcs/structures/widget/spk_scroll_area.cpp` - Lines 119-121 - Add a test that reaches this uncovered block: `bar.activate();`.
- `srcs/structures/widget/spk_scroll_area.cpp` - Line 31 (cols 12-37) - Add branch coverage for `width > _scrollBarWidth`; missing false outcome.
- `srcs/structures/widget/spk_scroll_area.cpp` - Line 36 (cols 13-39) - Add branch coverage for `height > _scrollBarWidth`; missing false outcome.
- `srcs/structures/widget/spk_scroll_area.cpp` - Line 61 (cols 27-40) - Add branch coverage for `content.x > 0`; missing false outcome.
- `srcs/structures/widget/spk_scroll_area.cpp` - Line 65 (cols 27-40) - Add branch coverage for `content.y > 0`; missing false outcome.
- `srcs/structures/widget/spk_scroll_area.cpp` - Line 94 (cols 7-47) - Add branch coverage for `_verticalScrollBar.isActivated() == true`; missing false outcome.
- `srcs/structures/widget/spk_scroll_area.cpp` - Line 118 (cols 7-22) - Add branch coverage for `p_state == true`; missing true outcome.

### srcs/structures/widget/spk_scroll_bar.cpp

Coverage: lines 88.98% (105/118, missing 13); branches 80.00% (16/20, missing 4); functions 86.36% (19/22, missing 3).

- `srcs/structures/widget/spk_scroll_bar.cpp` - Line 83 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Line 94 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 127-128 - Add a test that reaches this uncovered block: `_step = p_step;`.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 151-153 - Add a test that reaches this return path: `return _negativeButton;`.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 161-163 - Add a test that reaches this return path: `return _positiveButton;`.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Lines 171-173 - Add a test that reaches this return path: `return _sliderBar;`.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Line 56 (cols 11-35) - Add branch coverage for `_sliderBar.orientation`; missing false outcome.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Line 71 (cols 11-35) - Add branch coverage for `_sliderBar.orientation`; missing false outcome.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Line 77 (cols 5-42) - Add branch coverage for `geometry().width() > buttonSize * 2`; missing false outcome.
- `srcs/structures/widget/spk_scroll_bar.cpp` - Line 122 (cols 25-38) - Add branch coverage for `p_step > 1.0f`; missing false outcome.

### srcs/structures/widget/spk_slider_bar.cpp

Coverage: lines 83.49% (182/218, missing 36); branches 68.75% (33/48, missing 15); functions 85.71% (24/28, missing 4).

- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 49-51 - Add a test that reaches this uncovered block: `_background.applyStyle(p_style);`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 77 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 90 - Add a test that reaches this uncovered block: `}`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 116-118 - Add a test that reaches this return path: `return _ratio;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 131-133 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 156-158 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 182-184 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 193-195 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 206-208 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 252-255 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 296-298 - Add a test that reaches this return path: `return _background;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 301-303 - Add a test that reaches this return path: `return _background;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Lines 311-313 - Add a test that reaches this return path: `return _body;`.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 63 (cols 11-23) - Add branch coverage for `_orientation`; missing false outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 107 (cols 4-14) - Add branch coverage for `horizontal`; missing false outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 111 (cols 4-14) - Add branch coverage for `horizontal`; missing false outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 115 (cols 7-20) - Add branch coverage for `range <= 0.0f`; missing true outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 121 (cols 4-14) - Add branch coverage for `horizontal`; missing false outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 130 (cols 7-42) - Add branch coverage for `p_event->button != spk::Mouse::Left`; missing true outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 155 (cols 7-42) - Add branch coverage for `p_event->button != spk::Mouse::Left`; missing true outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 155 (cols 46-65) - Add branch coverage for `_isDragged == false`; missing true outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 173 (cols 4-50) - Add branch coverage for `_orientation == spk::Orientation::Horizontal`; missing false outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 177 (cols 4-50) - Add branch coverage for `_orientation == spk::Orientation::Horizontal`; missing false outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 181 (cols 7-20) - Add branch coverage for `range <= 0.0f`; missing true outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 187 (cols 4-50) - Add branch coverage for `_orientation == spk::Orientation::Horizontal`; missing false outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 192 (cols 7-25) - Add branch coverage for `newRatio == _ratio`; missing true outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 205 (cols 7-36) - Add branch coverage for `_orientation == p_orientation`; missing true outcome.
- `srcs/structures/widget/spk_slider_bar.cpp` - Line 251 (cols 7-29) - Add branch coverage for `_maxValue == _minValue`; missing true outcome.

### srcs/structures/widget/spk_text_area.cpp

Coverage: lines 79.41% (243/306, missing 63); branches 66.25% (53/80, missing 27); functions 86.49% (32/37, missing 5).

- `srcs/structures/widget/spk_text_area.cpp` - Lines 111-113 - Add a test that reaches this return path: `return 0;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 130-132 - Add a test that reaches this return path: `return result;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 137-140 - Add a loop/control-flow test that reaches this block: `continue;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 185-187 - Add a test that reaches this return path: `return builder.build();`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 204-206 - Add a test that reaches this uncovered block: `lineTop = geometry().top() + (static_cast<int>(geometry().height()) - static_cast<int>(blockHeight)) / 2;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 208-210 - Add a test that reaches this uncovered block: `lineTop = geometry().bottom() - static_cast<int>(blockHeight);`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 218-220 - Add a test that reaches this uncovered block: `anchorX = geometry().left() + static_cast<int>(geometry().width() / 2);`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 222-224 - Add a test that reaches this uncovered block: `anchorX = geometry().right();`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 247-249 - Add a test that reaches this return path: `return {0, 0};`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 294-296 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 309-311 - Add a test that reaches this uncovered block: `_glyphColor = p_color;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 325-329 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 331-333 - Add a test that reaches this uncovered block: `_depth = p_depth;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 338-340 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 361-363 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 374-376 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 404-406 - Add a test that reaches this return path: `return _font;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 414-416 - Add a test that reaches this return path: `return _glyphColor;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 419-421 - Add a test that reaches this return path: `return _outlineColor;`.
- `srcs/structures/widget/spk_text_area.cpp` - Lines 424-426 - Add a test that reaches this return path: `return _depth;`.
- `srcs/structures/widget/spk_text_area.cpp` - Line 15 (cols 7-28) - Add branch coverage for `p_left.g == p_right.g`; missing false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 16 (cols 7-28) - Add branch coverage for `p_left.b == p_right.b`; missing false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 17 (cols 7-28) - Add branch coverage for `p_left.a == p_right.a`; missing false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 25 (cols 10-32) - Add branch coverage for `start <= p_text.size`; missing false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 110 (cols 7-23) - Add branch coverage for `_font == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 129 (cols 7-23) - Add branch coverage for `_font == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 136 (cols 8-33) - Add branch coverage for `paragraph.empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 171 (cols 8-36) - Add branch coverage for `currentLine.empty() == false`; missing false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 184 (cols 7-33) - Add branch coverage for `geometry().empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 189 (cols 7-23) - Add branch coverage for `_font == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 197 (cols 4-27) - Add branch coverage for `lines.empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 203 (cols 7-61) - Add branch coverage for `_verticalAlignment == spk::VerticalAlignment::Centered`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 207 (cols 12-62) - Add branch coverage for `_verticalAlignment == spk::VerticalAlignment::Down`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 214 (cols 8-29) - Add branch coverage for `line.empty() == false`; missing false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 217 (cols 9-67) - Add branch coverage for `_horizontalAlignment == spk::HorizontalAlignment::Centered`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 221 (cols 14-69) - Add branch coverage for `_horizontalAlignment == spk::HorizontalAlignment::Right`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 246 (cols 7-23) - Add branch coverage for `_font == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 267 (cols 4-27) - Add branch coverage for `lines.empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 293 (cols 7-30) - Add branch coverage for `_textSize == p_textSize`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 304 (cols 7-46) - Add branch coverage for `sameColor(_glyphColor, p_color) == true`; missing false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 326 (cols 7-24) - Add branch coverage for `_depth == p_depth`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 337 (cols 7-22) - Add branch coverage for `_text == p_text`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 360 (cols 7-36) - Add branch coverage for `_linePadding == p_linePadding`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 373 (cols 7-52) - Add branch coverage for `_horizontalAlignment == p_horizontalAlignment`; missing true outcome.
- `srcs/structures/widget/spk_text_area.cpp` - Line 373 (cols 56-97) - Add branch coverage for `_verticalAlignment == p_verticalAlignment`; missing true outcome and false outcome.

### srcs/structures/widget/spk_text_edit.cpp

Coverage: lines 76.88% (409/532, missing 123); branches 63.82% (97/152, missing 55); functions 81.67% (49/60, missing 11).

- `srcs/structures/widget/spk_text_edit.cpp` - Lines 32-49 - Add a state-changing test that executes this block: `result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));`.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 61 - Add a test that reaches this uncovered block: `spk::Vector2UInt result = {0, 0};`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 63-68 - Add a test that reaches this uncovered block: `if (_font != nullptr && _placeholder.empty() == false)`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 70-71 - Add a test that reaches this return path: `return result;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 130-132 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 173-175 - Add a test that reaches this return path: `return builder.build();`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 239-240 - Add a test that reaches this uncovered block: `const bool newRenderCursor = (p_tick.timestamp.milliseconds() / 250) % 2 == 0;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 242-250 - Add a test that reaches this uncovered block: `if (newRenderCursor != _renderCursor)`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 279-281 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 290-293 - Add a test that reaches this uncovered block: `releaseFocus(FocusType::Keyboard);`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 299-301 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 321-322 - Add a switch/enum test case that reaches this branch: `default:`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 347-349 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 353-356 - Add a test that reaches this uncovered block: `_lowerCursor = _cursor;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 363-365 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 402-404 - Add a test that reaches this return path: `return _validate(_text);`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 409-411 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 422-424 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 430-433 - Add a test that reaches this uncovered block: `_lowerCursor = (_lowerCursor > 0 && _cursor > 0) ? _cursor - 1 : 0;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 469-471 - Add a negative-path test that triggers and asserts this exception: `TextEdit sprite sheet must contain a 3x3 grid`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 528-530 - Add a test that reaches this uncovered block: `_glyphColor = p_color;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 544-548 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 550-552 - Add a test that reaches this uncovered block: `_cursorColor = p_color;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 555-559 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 561-563 - Add a test that reaches this uncovered block: `_depth = p_depth;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 599-601 - Add a test that reaches this return path: `return;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 670-672 - Add a test that reaches this return path: `return _spriteSheet;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 675-677 - Add a test that reaches this return path: `return _cornerSize;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 680-682 - Add a test that reaches this return path: `return _font;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 690-692 - Add a test that reaches this return path: `return _glyphColor;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 695-697 - Add a test that reaches this return path: `return _outlineColor;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 700-702 - Add a test that reaches this return path: `return _depth;`.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 60 - Add a test that calls this currently unexecuted function: `configureMinimalSizeGenerator([this]() {`.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 16 (cols 7-28) - Add branch coverage for `p_left.g == p_right.g`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 17 (cols 7-28) - Add branch coverage for `p_left.b == p_right.b`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 18 (cols 7-28) - Add branch coverage for `p_left.a == p_right.a`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 28 (cols 8-25) - Add branch coverage for `codepoint <= 0x7F`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 32 (cols 13-31) - Add branch coverage for `codepoint <= 0x7FF`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 37 (cols 13-32) - Add branch coverage for `codepoint <= 0xFFFF`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 63 (cols 8-24) - Add branch coverage for `_font != nullptr`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 63 (cols 28-57) - Add branch coverage for `_placeholder.empty() == false`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 129 (cols 7-23) - Add branch coverage for `_font == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 137 (cols 7-37) - Add branch coverage for `_needLowerCursorUpdate == true`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 142-144 (cols 8-30) - Add branch coverage for `_font->computeStringSize( textToRender.substr(_lowerCursor - 1, _cursor - _lowerCursor + 1), _textSize)`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 152 (cols 7-38) - Add branch coverage for `_needHigherCursorUpdate == true`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Lines 157-159 (cols 8-30) - Add branch coverage for `_font->computeStringSize( textToRender.substr(_lowerCursor, _higherCursor - _lowerCursor + 1), _textSize)`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 172 (cols 7-33) - Add branch coverage for `geometry().empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 177 (cols 7-30) - Add branch coverage for `_spriteSheet != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 192 (cols 7-23) - Add branch coverage for `_font != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 192 (cols 27-55) - Add branch coverage for `visibleText.empty() == false`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 210 (cols 7-23) - Add branch coverage for `_font != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 211 (cols 4-26) - Add branch coverage for `_isEditEnabled == true`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 212 (cols 4-25) - Add branch coverage for `_renderCursor == true`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 215 (cols 33-58) - Add branch coverage for `_cursor >= _lowerCursor`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 242 (cols 7-39) - Add branch coverage for `newRenderCursor != _renderCursor`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 245 (cols 8-45) - Add branch coverage for `hasFocus(FocusType::Keyboard) == true`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 278 (cols 7-42) - Add branch coverage for `p_event->button != spk::Mouse::Left`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 289 (cols 12-49) - Add branch coverage for `hasFocus(FocusType::Keyboard) == true`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 298 (cols 7-30) - Add branch coverage for `_isEditEnabled == false`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 298 (cols 34-72) - Add branch coverage for `hasFocus(FocusType::Keyboard) == false`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 321 (cols 3-10) - Add branch coverage for `default`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 346 (cols 7-19) - Add branch coverage for `_cursor == 0`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 352 (cols 7-29) - Add branch coverage for `_cursor < _lowerCursor`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 362 (cols 7-30) - Add branch coverage for `_cursor >= _text.size`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 368 (cols 7-31) - Add branch coverage for `_cursor >= _higherCursor`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 408 (cols 7-30) - Add branch coverage for `_cursor >= _text.size`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 421 (cols 7-28) - Add branch coverage for `_text.empty() == true`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 421 (cols 32-44) - Add branch coverage for `_cursor == 0`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 429 (cols 7-30) - Add branch coverage for `_cursor <= _lowerCursor`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 431 (cols 20-36) - Add branch coverage for `_lowerCursor > 0`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 431 (cols 40-51) - Add branch coverage for `_cursor > 0`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 451 (cols 7-30) - Add branch coverage for `_cursor > _higherCursor`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 468 (cols 7-58) - Add branch coverage for `p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 3}`; missing true outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 523 (cols 7-46) - Add branch coverage for `sameColor(_glyphColor, p_color) == true`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 545 (cols 7-47) - Add branch coverage for `sameColor(_cursorColor, p_color) == true`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 556 (cols 7-24) - Add branch coverage for `_depth == p_depth`; missing true outcome and false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 583 (cols 7-28) - Add branch coverage for `_text.empty() == true`; missing false outcome.
- `srcs/structures/widget/spk_text_edit.cpp` - Line 598 (cols 7-29) - Add branch coverage for `_isObscured == p_state`; missing true outcome.

### srcs/structures/widget/spk_text_label.cpp

Coverage: lines 97.25% (212/218, missing 6); branches 89.58% (43/48, missing 5); functions 100.00% (33/33, missing 0).

- `srcs/structures/widget/spk_text_label.cpp` - Line 85 (cols 8-24) - Add branch coverage for `_font != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_text_label.cpp` - Line 127 (cols 7-23) - Add branch coverage for `_font != nullptr`; missing false outcome.

### srcs/structures/widget/spk_widget.cpp

Coverage: lines 94.96% (433/456, missing 23); branches 86.76% (59/68, missing 9); functions 88.89% (64/72, missing 8).

- `srcs/structures/widget/spk_widget.cpp` - Lines 42-43 - Add a test that calls the empty `Widget::applyStyle` default hook.
- `srcs/structures/widget/spk_widget.cpp` - Lines 74-76 - Add a test that reaches this uncovered block: `(void)p_event;`.
- `srcs/structures/widget/spk_widget.cpp` - Lines 78-80 - Add a test that reaches this uncovered block: `(void)p_event;`.
- `srcs/structures/widget/spk_widget.cpp` - Lines 82-84 - Add a test that reaches this uncovered block: `(void)p_event;`.
- `srcs/structures/widget/spk_widget.cpp` - Lines 86-88 - Add a test that reaches this uncovered block: `(void)p_event;`.
- `srcs/structures/widget/spk_widget.cpp` - Lines 114-116 - Add a test that reaches this uncovered block: `(void)p_event;`.
- `srcs/structures/widget/spk_widget.cpp` - Lines 126-128 - Add a test that reaches this uncovered block: `(void)p_event;`.
- `srcs/structures/widget/spk_widget.cpp` - Lines 251-253 - Add a test that reaches this uncovered block: `setGeometry(spk::Rect2D(_geometry.anchor + p_delta, _geometry.size));`.
- `srcs/structures/widget/spk_widget.cpp` - Line 213 (cols 8-24) - Add branch coverage for `child == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 213 (cols 60-88) - Add branch coverage for `child->_geometry.size.y == 0`; missing true outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 268 (cols 7-27) - Add branch coverage for `_viewport != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 283 (cols 8-24) - Add branch coverage for `child != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 311 (cols 8-24) - Add branch coverage for `child != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 345 (cols 39-61) - Add branch coverage for `_renderUnit == nullptr`; missing true outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 364 (cols 7-27) - Add branch coverage for `_viewport != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 386 (cols 8-24) - Add branch coverage for `child != nullptr`; missing false outcome.
- `srcs/structures/widget/spk_widget.cpp` - Line 404 (cols 8-24) - Add branch coverage for `child != nullptr`; missing false outcome.

### srcs/structures/widget/spk_widget_style.cpp

Coverage: lines 93.15% (231/248, missing 17); branches 77.78% (28/36, missing 8); functions 93.02% (40/43, missing 3).

- `srcs/structures/widget/spk_widget_style.cpp` - Line 177 (cols 9-27) - Add branch coverage for `p_style != nullptr`; missing false outcome.

### srcs/utils/spk_winapi_helpers.cpp

Coverage: lines 89.09% (49/55, missing 6); branches 62.50% (10/16, missing 6); functions 100.00% (4/4, missing 0).

- `srcs/utils/spk_winapi_helpers.cpp` - Lines 16-18 - Add a negative-path test that triggers this exception block: `throwLastError("MultiByteToWideChar");`.
- `srcs/utils/spk_winapi_helpers.cpp` - Lines 34-36 - Add a negative-path test that triggers this exception block: `throwLastError("WideCharToMultiByte");`.
- `srcs/utils/spk_winapi_helpers.cpp` - Line 15 (cols 7-16) - Add branch coverage for `size <= 0`; missing true outcome.
- `srcs/utils/spk_winapi_helpers.cpp` - Line 33 (cols 7-16) - Add branch coverage for `size <= 0`; missing true outcome.
- `srcs/utils/spk_winapi_helpers.cpp` - Line 58 (cols 7-21) - Add branch coverage for `errorCode != 0`; missing false outcome.
- `srcs/utils/spk_winapi_helpers.cpp` - Line 63 (cols 7-16) - Add branch coverage for `size != 0`; missing false outcome.
- `srcs/utils/spk_winapi_helpers.cpp` - Line 63 (cols 20-37) - Add branch coverage for `buffer != nullptr`; missing false outcome.
- `srcs/utils/spk_winapi_helpers.cpp` - Line 68 (cols 7-24) - Add branch coverage for `buffer != nullptr`; missing false outcome.
