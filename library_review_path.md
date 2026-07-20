# Sparkle library review path

This is the dependency-ordered reading path for the current Sparkle implementation.

## Scope

The implementation surface currently contains:

- 257 `.hpp` headers under `includes/`
- 1 extensionless forwarding include, `includes/sparkle`, which CMake explicitly installs with the headers
- 179 `.cpp` sources under `srcs/`
- 437 implementation files in total

The path below is generated from direct internal `#include` relationships. Standard-library, third-party, generated, and test dependencies are not used to order the waves. A file appears only after all of its direct Sparkle include dependencies have appeared. Files in the same wave can be reviewed in any order.

This is an order for understanding the code, not a claim that the current architecture is correct. Existing documentation is out of scope for this inventory and was not modified; treat it as untrusted until the review is complete. Use [library_review_notes/README.md](<library_review_notes/README.md>) as the review record index.

## How to use the path

For every entry:

1. Read the complete header or source file, including private helpers and conditional compilation.
2. Read the files in earlier waves that it directly includes.
3. Record the type's purpose, invariants, ownership/lifetime rules, thread assumptions, error behavior, and suspicious coupling in its note file.
4. Read the related unit/visual/performance tests after the implementation, then note missing or misleading coverage.
5. Do not mark a wave complete until every entry in it has a status in its note file.

When a `.cpp` does not have a same-named `.hpp`, treat its direct includes as its contract boundary. This is intentional for several implementation files that share a broader header.

## Dependency waves

### Wave 0 — Foundation: files with no internal Sparkle include dependencies

**40 files**

- [includes/structures/container/spk_binary_field.hpp](<includes/structures/container/spk_binary_field.hpp>)
- [includes/structures/container/spk_cached_data.hpp](<includes/structures/container/spk_cached_data.hpp>)
- [includes/structures/container/spk_flags.hpp](<includes/structures/container/spk_flags.hpp>)
- [includes/structures/container/spk_object_pool.hpp](<includes/structures/container/spk_object_pool.hpp>)
- [includes/structures/container/spk_uuid.hpp](<includes/structures/container/spk_uuid.hpp>)
- [includes/structures/container/spk_weighted_pool.hpp](<includes/structures/container/spk_weighted_pool.hpp>)
- [includes/structures/design_pattern/spk_blockable_trait.hpp](<includes/structures/design_pattern/spk_blockable_trait.hpp>)
- [includes/structures/design_pattern/spk_inherence_trait.hpp](<includes/structures/design_pattern/spk_inherence_trait.hpp>)
- [includes/structures/design_pattern/spk_synchronizable_trait.hpp](<includes/structures/design_pattern/spk_synchronizable_trait.hpp>)
- [includes/structures/game_engine/rendering/spk_scene_render_passes.hpp](<includes/structures/game_engine/rendering/spk_scene_render_passes.hpp>)
- [includes/structures/game_engine/rendering/spk_scene_render_priorities.hpp](<includes/structures/game_engine/rendering/spk_scene_render_priorities.hpp>)
- [includes/structures/graphics/geometry/spk_color.hpp](<includes/structures/graphics/geometry/spk_color.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_object.hpp](<includes/structures/graphics/opengl/spk_opengl_object.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_uniform.hpp](<includes/structures/graphics/opengl/spk_opengl_uniform.hpp>)
- [includes/structures/graphics/rendering/command/spk_opengl_draw_state_guard.hpp](<includes/structures/graphics/rendering/command/spk_opengl_draw_state_guard.hpp>)
- [includes/structures/graphics/rendering/command/spk_render_command.hpp](<includes/structures/graphics/rendering/command/spk_render_command.hpp>)
- [includes/structures/graphics/rendering/spk_render_frame_build_context.hpp](<includes/structures/graphics/rendering/spk_render_frame_build_context.hpp>)
- [includes/structures/graphics/rendering/spk_scene_gpu_bindings.hpp](<includes/structures/graphics/rendering/spk_scene_gpu_bindings.hpp>)
- [includes/structures/graphics/spk_primitive.hpp](<includes/structures/graphics/spk_primitive.hpp>)
- [includes/structures/math/spk_deterministic_random.hpp](<includes/structures/math/spk_deterministic_random.hpp>)
- [includes/structures/math/spk_perlin.hpp](<includes/structures/math/spk_perlin.hpp>)
- [includes/structures/system/thread/spk_thread_safe_queue.hpp](<includes/structures/system/thread/spk_thread_safe_queue.hpp>)
- [includes/structures/system/thread/spk_worker_pool.hpp](<includes/structures/system/thread/spk_worker_pool.hpp>)
- [includes/structures/system/win32/spk_winapi_class.hpp](<includes/structures/system/win32/spk_winapi_class.hpp>)
- [includes/structures/system/win32/spk_winapi_cursor.hpp](<includes/structures/system/win32/spk_winapi_cursor.hpp>)
- [includes/structures/voxel/spk_voxel_enums.hpp](<includes/structures/voxel/spk_voxel_enums.hpp>)
- [includes/structures/voxel/spk_voxel_ids.hpp](<includes/structures/voxel/spk_voxel_ids.hpp>)
- [includes/structures/widget/rendering/spk_widget_render_passes.hpp](<includes/structures/widget/rendering/spk_widget_render_passes.hpp>)
- [includes/structures/widget/rendering/spk_widget_render_priorities.hpp](<includes/structures/widget/rendering/spk_widget_render_priorities.hpp>)
- [includes/type/spk_concepts.hpp](<includes/type/spk_concepts.hpp>)
- [includes/type/spk_constants.hpp](<includes/type/spk_constants.hpp>)
- [includes/type/spk_horizontal_alignment.hpp](<includes/type/spk_horizontal_alignment.hpp>)
- [includes/type/spk_input_state.hpp](<includes/type/spk_input_state.hpp>)
- [includes/type/spk_orientation.hpp](<includes/type/spk_orientation.hpp>)
- [includes/type/spk_padding.hpp](<includes/type/spk_padding.hpp>)
- [includes/type/spk_time_unit.hpp](<includes/type/spk_time_unit.hpp>)
- [includes/type/spk_vertical_alignment.hpp](<includes/type/spk_vertical_alignment.hpp>)
- [includes/utils/spk_math.hpp](<includes/utils/spk_math.hpp>)
- [includes/utils/spk_winapi_helpers.hpp](<includes/utils/spk_winapi_helpers.hpp>)
- [srcs/structures/graphics/rendering/context/spk_render_context.cpp](<srcs/structures/graphics/rendering/context/spk_render_context.cpp>)

**Wave 0 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 1 — First dependents of the foundation

**38 files**

- [includes/structures/container/spk_json_concepts.hpp](<includes/structures/container/spk_json_concepts.hpp>)
- [includes/structures/design_pattern/spk_contract_provider.hpp](<includes/structures/design_pattern/spk_contract_provider.hpp>)
- [includes/structures/game_engine/spk_component_container.hpp](<includes/structures/game_engine/spk_component_container.hpp>)
- [includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp](<includes/structures/graphics/opengl/spk_cached_opengl_object_collection.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_buffer.hpp](<includes/structures/graphics/opengl/spk_opengl_buffer.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_clear_command.hpp](<includes/structures/graphics/opengl/spk_opengl_clear_command.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_framebuffer_object.hpp](<includes/structures/graphics/opengl/spk_opengl_framebuffer_object.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_primitive.hpp](<includes/structures/graphics/opengl/spk_opengl_primitive.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_program.hpp](<includes/structures/graphics/opengl/spk_opengl_program.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_uniform_location.hpp](<includes/structures/graphics/opengl/spk_opengl_uniform_location.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_vertex_array.hpp](<includes/structures/graphics/opengl/spk_opengl_vertex_array.hpp>)
- [includes/structures/graphics/rendering/command/spk_directional_shadow_update_render_command.hpp](<includes/structures/graphics/rendering/command/spk_directional_shadow_update_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_render_unit_render_command.hpp](<includes/structures/graphics/rendering/command/spk_render_unit_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_scene_lighting_update_render_command.hpp](<includes/structures/graphics/rendering/command/spk_scene_lighting_update_render_command.hpp>)
- [includes/structures/graphics/rendering/pass/spk_render_pass_clear.hpp](<includes/structures/graphics/rendering/pass/spk_render_pass_clear.hpp>)
- [includes/structures/graphics/rendering/unit/spk_render_unit.hpp](<includes/structures/graphics/rendering/unit/spk_render_unit.hpp>)
- [includes/structures/math/spk_approx_value.hpp](<includes/structures/math/spk_approx_value.hpp>)
- [includes/structures/system/device/input/spk_keyboard.hpp](<includes/structures/system/device/input/spk_keyboard.hpp>)
- [includes/structures/system/thread/spk_thread_safe_contract.hpp](<includes/structures/system/thread/spk_thread_safe_contract.hpp>)
- [includes/structures/system/time/spk_duration.hpp](<includes/structures/system/time/spk_duration.hpp>)
- [includes/structures/voxel/spk_voxel_cell.hpp](<includes/structures/voxel/spk_voxel_cell.hpp>)
- [includes/structures/widget/rendering/spk_widget_render_build_context.hpp](<includes/structures/widget/rendering/spk_widget_render_build_context.hpp>)
- [srcs/structures/container/spk_binary_field.cpp](<srcs/structures/container/spk_binary_field.cpp>)
- [srcs/structures/container/spk_uuid.cpp](<srcs/structures/container/spk_uuid.cpp>)
- [srcs/structures/design_pattern/spk_blockable_trait.cpp](<srcs/structures/design_pattern/spk_blockable_trait.cpp>)
- [srcs/structures/design_pattern/spk_synchronizable_trait.cpp](<srcs/structures/design_pattern/spk_synchronizable_trait.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_uniform.cpp](<srcs/structures/graphics/opengl/spk_opengl_uniform.cpp>)
- [srcs/structures/math/spk_perlin.cpp](<srcs/structures/math/spk_perlin.cpp>)
- [srcs/structures/system/thread/spk_worker_pool.cpp](<srcs/structures/system/thread/spk_worker_pool.cpp>)
- [srcs/structures/system/win32/spk_winapi_class.cpp](<srcs/structures/system/win32/spk_winapi_class.cpp>)
- [srcs/type/spk_constants.cpp](<srcs/type/spk_constants.cpp>)
- [srcs/type/spk_horizontal_alignment.cpp](<srcs/type/spk_horizontal_alignment.cpp>)
- [srcs/type/spk_input_state.cpp](<srcs/type/spk_input_state.cpp>)
- [srcs/type/spk_math.cpp](<srcs/type/spk_math.cpp>)
- [srcs/type/spk_orientation.cpp](<srcs/type/spk_orientation.cpp>)
- [srcs/type/spk_time_unit.cpp](<srcs/type/spk_time_unit.cpp>)
- [srcs/type/spk_vertical_alignment.cpp](<srcs/type/spk_vertical_alignment.cpp>)
- [srcs/utils/spk_winapi_helpers.cpp](<srcs/utils/spk_winapi_helpers.cpp>)

**Wave 1 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 2 — Reusable container, trait, math, system, and graphics dependents

**19 files**

- [includes/structures/container/spk_json_object.hpp](<includes/structures/container/spk_json_object.hpp>)
- [includes/structures/container/spk_observable_value.hpp](<includes/structures/container/spk_observable_value.hpp>)
- [includes/structures/design_pattern/spk_activable_trait.hpp](<includes/structures/design_pattern/spk_activable_trait.hpp>)
- [includes/structures/design_pattern/spk_priorizable_trait.hpp](<includes/structures/design_pattern/spk_priorizable_trait.hpp>)
- [includes/structures/game_engine/spk_component_store.hpp](<includes/structures/game_engine/spk_component_store.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_sampler_object.hpp](<includes/structures/graphics/opengl/spk_opengl_sampler_object.hpp>)
- [includes/structures/graphics/rendering/snapshot/spk_render_snapshot.hpp](<includes/structures/graphics/rendering/snapshot/spk_render_snapshot.hpp>)
- [includes/structures/graphics/rendering/unit/spk_render_unit_builder.hpp](<includes/structures/graphics/rendering/unit/spk_render_unit_builder.hpp>)
- [includes/structures/graphics/spk_buffer_object.hpp](<includes/structures/graphics/spk_buffer_object.hpp>)
- [includes/structures/graphics/spk_program.hpp](<includes/structures/graphics/spk_program.hpp>)
- [includes/structures/math/spk_vector2.hpp](<includes/structures/math/spk_vector2.hpp>)
- [includes/structures/system/time/spk_timestamp.hpp](<includes/structures/system/time/spk_timestamp.hpp>)
- [srcs/structures/game_engine/spk_component_container.cpp](<srcs/structures/game_engine/spk_component_container.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_buffer.cpp](<srcs/structures/graphics/opengl/spk_opengl_buffer.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_primitive.cpp](<srcs/structures/graphics/opengl/spk_opengl_primitive.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_vertex_array.cpp](<srcs/structures/graphics/opengl/spk_opengl_vertex_array.cpp>)
- [srcs/structures/graphics/rendering/command/spk_render_unit_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_render_unit_render_command.cpp>)
- [srcs/structures/system/device/input/spk_keyboard.cpp](<srcs/structures/system/device/input/spk_keyboard.cpp>)
- [srcs/structures/system/time/spk_duration.cpp](<srcs/structures/system/time/spk_duration.cpp>)

**Wave 2 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 3 — Reusable cross-subsystem value and service types

**25 files**

- [includes/structures/application/module/spk_render_module.hpp](<includes/structures/application/module/spk_render_module.hpp>)
- [includes/structures/container/spk_grid_2d.hpp](<includes/structures/container/spk_grid_2d.hpp>)
- [includes/structures/container/spk_json_reader.hpp](<includes/structures/container/spk_json_reader.hpp>)
- [includes/structures/game_engine/spk_component.hpp](<includes/structures/game_engine/spk_component.hpp>)
- [includes/structures/game_engine/spk_component_registry.hpp](<includes/structures/game_engine/spk_component_registry.hpp>)
- [includes/structures/graphics/geometry/spk_polygon_2d.hpp](<includes/structures/graphics/geometry/spk_polygon_2d.hpp>)
- [includes/structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp](<includes/structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp>)
- [includes/structures/graphics/spk_index_buffer_object.hpp](<includes/structures/graphics/spk_index_buffer_object.hpp>)
- [includes/structures/graphics/spk_shader_storage_buffer_object.hpp](<includes/structures/graphics/spk_shader_storage_buffer_object.hpp>)
- [includes/structures/graphics/spk_texture.hpp](<includes/structures/graphics/spk_texture.hpp>)
- [includes/structures/graphics/spk_uniform.hpp](<includes/structures/graphics/spk_uniform.hpp>)
- [includes/structures/graphics/spk_uniform_buffer_object.hpp](<includes/structures/graphics/spk_uniform_buffer_object.hpp>)
- [includes/structures/graphics/spk_vertex_buffer_object.hpp](<includes/structures/graphics/spk_vertex_buffer_object.hpp>)
- [includes/structures/math/spk_curve_interpolator.hpp](<includes/structures/math/spk_curve_interpolator.hpp>)
- [includes/structures/math/spk_rect_2d.hpp](<includes/structures/math/spk_rect_2d.hpp>)
- [includes/structures/math/spk_vector3.hpp](<includes/structures/math/spk_vector3.hpp>)
- [includes/structures/system/device/input/spk_mouse.hpp](<includes/structures/system/device/input/spk_mouse.hpp>)
- [includes/structures/system/spk_command_line_parser.hpp](<includes/structures/system/spk_command_line_parser.hpp>)
- [includes/structures/system/time/spk_chronometer.hpp](<includes/structures/system/time/spk_chronometer.hpp>)
- [includes/utils/spk_time_utils.hpp](<includes/utils/spk_time_utils.hpp>)
- [srcs/structures/container/spk_json_object.cpp](<srcs/structures/container/spk_json_object.cpp>)
- [srcs/structures/design_pattern/spk_activable_trait.cpp](<srcs/structures/design_pattern/spk_activable_trait.cpp>)
- [srcs/structures/game_engine/spk_component_store.cpp](<srcs/structures/game_engine/spk_component_store.cpp>)
- [srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp](<srcs/structures/graphics/rendering/unit/spk_render_unit_builder.cpp>)
- [srcs/structures/system/time/spk_timestamp.cpp](<srcs/structures/system/time/spk_timestamp.cpp>)

**Wave 3 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 4 — Resource, geometry, device, voxel, and render-building types

**42 files**

- [includes/structures/container/spk_definition_registry.hpp](<includes/structures/container/spk_definition_registry.hpp>)
- [includes/structures/game_engine/spk_component_2d.hpp](<includes/structures/game_engine/spk_component_2d.hpp>)
- [includes/structures/game_engine/spk_component_3d.hpp](<includes/structures/game_engine/spk_component_3d.hpp>)
- [includes/structures/game_engine/spk_entity.hpp](<includes/structures/game_engine/spk_entity.hpp>)
- [includes/structures/graphics/geometry/spk_polygon_3d.hpp](<includes/structures/graphics/geometry/spk_polygon_3d.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_texture.hpp](<includes/structures/graphics/opengl/spk_opengl_texture.hpp>)
- [includes/structures/graphics/spk_directional_light.hpp](<includes/structures/graphics/spk_directional_light.hpp>)
- [includes/structures/graphics/spk_gpu_data_buffer_center.hpp](<includes/structures/graphics/spk_gpu_data_buffer_center.hpp>)
- [includes/structures/graphics/spk_sampler_object.hpp](<includes/structures/graphics/spk_sampler_object.hpp>)
- [includes/structures/graphics/spk_vertex_array_object.hpp](<includes/structures/graphics/spk_vertex_array_object.hpp>)
- [includes/structures/graphics/texture/spk_font.hpp](<includes/structures/graphics/texture/spk_font.hpp>)
- [includes/structures/graphics/texture/spk_image.hpp](<includes/structures/graphics/texture/spk_image.hpp>)
- [includes/structures/math/spk_cellular_automata.hpp](<includes/structures/math/spk_cellular_automata.hpp>)
- [includes/structures/math/spk_poisson_disk.hpp](<includes/structures/math/spk_poisson_disk.hpp>)
- [includes/structures/math/spk_quaternion.hpp](<includes/structures/math/spk_quaternion.hpp>)
- [includes/structures/math/spk_ray_3d.hpp](<includes/structures/math/spk_ray_3d.hpp>)
- [includes/structures/math/spk_vector4.hpp](<includes/structures/math/spk_vector4.hpp>)
- [includes/structures/system/device/window/spk_surface_state.hpp](<includes/structures/system/device/window/spk_surface_state.hpp>)
- [includes/structures/system/device/window/spk_window_handle.hpp](<includes/structures/system/device/window/spk_window_handle.hpp>)
- [includes/structures/system/event/spk_events.hpp](<includes/structures/system/event/spk_events.hpp>)
- [includes/structures/system/event/spk_update_context.hpp](<includes/structures/system/event/spk_update_context.hpp>)
- [includes/structures/system/spk_profiler.hpp](<includes/structures/system/spk_profiler.hpp>)
- [includes/structures/system/time/spk_timer.hpp](<includes/structures/system/time/spk_timer.hpp>)
- [includes/structures/system/win32/spk_winapi_window.hpp](<includes/structures/system/win32/spk_winapi_window.hpp>)
- [includes/structures/voxel/spk_voxel_cell_lookup.hpp](<includes/structures/voxel/spk_voxel_cell_lookup.hpp>)
- [includes/structures/voxel/spk_voxel_chunk_streamer.hpp](<includes/structures/voxel/spk_voxel_chunk_streamer.hpp>)
- [includes/structures/voxel/spk_voxel_grid.hpp](<includes/structures/voxel/spk_voxel_grid.hpp>)
- [includes/structures/voxel/spk_voxel_orientation.hpp](<includes/structures/voxel/spk_voxel_orientation.hpp>)
- [includes/structures/widget/spk_resizable_element.hpp](<includes/structures/widget/spk_resizable_element.hpp>)
- [srcs/structures/container/spk_json_reader.cpp](<srcs/structures/container/spk_json_reader.cpp>)
- [srcs/structures/game_engine/spk_component_registry.cpp](<srcs/structures/game_engine/spk_component_registry.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_index_buffer_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_index_buffer_object.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_shader_storage_buffer_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_shader_storage_buffer_object.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_vertex_buffer_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_vertex_buffer_object.cpp>)
- [srcs/structures/graphics/rendering/command/spk_scene_lighting_update_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_scene_lighting_update_render_command.cpp>)
- [srcs/structures/graphics/rendering/snapshot/spk_render_snapshot_builder.cpp](<srcs/structures/graphics/rendering/snapshot/spk_render_snapshot_builder.cpp>)
- [srcs/structures/math/spk_curve_interpolator.cpp](<srcs/structures/math/spk_curve_interpolator.cpp>)
- [srcs/structures/math/spk_rect_2d.cpp](<srcs/structures/math/spk_rect_2d.cpp>)
- [srcs/structures/system/device/input/spk_mouse.cpp](<srcs/structures/system/device/input/spk_mouse.cpp>)
- [srcs/structures/system/spk_command_line_parser.cpp](<srcs/structures/system/spk_command_line_parser.cpp>)
- [srcs/structures/system/time/spk_chronometer.cpp](<srcs/structures/system/time/spk_chronometer.cpp>)
- [srcs/type/spk_time_utils.cpp](<srcs/type/spk_time_utils.cpp>)

**Wave 4 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 5 — Runtime resources and subsystem base classes

**28 files**

- [includes/structures/game_engine/spk_animation_2d.hpp](<includes/structures/game_engine/spk_animation_2d.hpp>)
- [includes/structures/game_engine/spk_light_3d.hpp](<includes/structures/game_engine/spk_light_3d.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_draw_arrays_command.hpp](<includes/structures/graphics/opengl/spk_opengl_draw_arrays_command.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_draw_elements_command.hpp](<includes/structures/graphics/opengl/spk_opengl_draw_elements_command.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_draw_elements_instanced_command.hpp](<includes/structures/graphics/opengl/spk_opengl_draw_elements_instanced_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp](<includes/structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp>)
- [includes/structures/graphics/spk_layout_buffer_object.hpp](<includes/structures/graphics/spk_layout_buffer_object.hpp>)
- [includes/structures/graphics/texture/spk_sprite_sheet.hpp](<includes/structures/graphics/texture/spk_sprite_sheet.hpp>)
- [includes/structures/math/spk_matrix.hpp](<includes/structures/math/spk_matrix.hpp>)
- [includes/structures/system/device/window/spk_frame.hpp](<includes/structures/system/device/window/spk_frame.hpp>)
- [includes/structures/voxel/spk_prefab.hpp](<includes/structures/voxel/spk_prefab.hpp>)
- [includes/structures/voxel/spk_voxel_ray_cast.hpp](<includes/structures/voxel/spk_voxel_ray_cast.hpp>)
- [includes/structures/voxel/spk_voxel_shape.hpp](<includes/structures/voxel/spk_voxel_shape.hpp>)
- [srcs/structures/game_engine/spk_component.cpp](<srcs/structures/game_engine/spk_component.cpp>)
- [srcs/structures/game_engine/spk_entity.cpp](<srcs/structures/game_engine/spk_entity.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_texture.cpp](<srcs/structures/graphics/opengl/spk_opengl_texture.cpp>)
- [srcs/structures/graphics/spk_gpu_data_buffer_center.cpp](<srcs/structures/graphics/spk_gpu_data_buffer_center.cpp>)
- [srcs/structures/graphics/spk_sampler_object.cpp](<srcs/structures/graphics/spk_sampler_object.cpp>)
- [srcs/structures/graphics/texture/spk_font.cpp](<srcs/structures/graphics/texture/spk_font.cpp>)
- [srcs/structures/graphics/texture/spk_font_true_type.cpp](<srcs/structures/graphics/texture/spk_font_true_type.cpp>)
- [srcs/structures/graphics/texture/spk_image.cpp](<srcs/structures/graphics/texture/spk_image.cpp>)
- [srcs/structures/math/spk_poisson_disk.cpp](<srcs/structures/math/spk_poisson_disk.cpp>)
- [srcs/structures/system/device/window/spk_surface_state.cpp](<srcs/structures/system/device/window/spk_surface_state.cpp>)
- [srcs/structures/system/spk_profiler.cpp](<srcs/structures/system/spk_profiler.cpp>)
- [srcs/structures/system/time/spk_timer.cpp](<srcs/structures/system/time/spk_timer.cpp>)
- [srcs/structures/system/win32/spk_winapi_cursor.cpp](<srcs/structures/system/win32/spk_winapi_cursor.cpp>)
- [srcs/structures/system/win32/spk_winapi_window.cpp](<srcs/structures/system/win32/spk_winapi_window.cpp>)
- [srcs/structures/widget/spk_resizable_element.cpp](<srcs/structures/widget/spk_resizable_element.cpp>)

**Wave 5 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 6 — Voxel composition and lower-level graphics/game-engine classes

**30 files**

- [includes/structures/game_engine/spk_camera_2d.hpp](<includes/structures/game_engine/spk_camera_2d.hpp>)
- [includes/structures/game_engine/spk_camera_3d.hpp](<includes/structures/game_engine/spk_camera_3d.hpp>)
- [includes/structures/game_engine/spk_transform_2d.hpp](<includes/structures/game_engine/spk_transform_2d.hpp>)
- [includes/structures/game_engine/spk_transform_3d.hpp](<includes/structures/game_engine/spk_transform_3d.hpp>)
- [includes/structures/graphics/geometry/spk_generic_mesh.hpp](<includes/structures/graphics/geometry/spk_generic_mesh.hpp>)
- [includes/structures/graphics/rendering/spk_scene_lighting_gpu_data.hpp](<includes/structures/graphics/rendering/spk_scene_lighting_gpu_data.hpp>)
- [includes/structures/graphics/rendering/state/spk_viewport.hpp](<includes/structures/graphics/rendering/state/spk_viewport.hpp>)
- [includes/structures/system/win32/spk_winapi_frame.hpp](<includes/structures/system/win32/spk_winapi_frame.hpp>)
- [includes/structures/voxel/spk_cross_plane_voxel_shape.hpp](<includes/structures/voxel/spk_cross_plane_voxel_shape.hpp>)
- [includes/structures/voxel/spk_cross_voxel_shape.hpp](<includes/structures/voxel/spk_cross_voxel_shape.hpp>)
- [includes/structures/voxel/spk_cube_voxel_shape.hpp](<includes/structures/voxel/spk_cube_voxel_shape.hpp>)
- [includes/structures/voxel/spk_cuboid_voxel_shape.hpp](<includes/structures/voxel/spk_cuboid_voxel_shape.hpp>)
- [includes/structures/voxel/spk_data_voxel_shape.hpp](<includes/structures/voxel/spk_data_voxel_shape.hpp>)
- [includes/structures/voxel/spk_hexa_plane_voxel_shape.hpp](<includes/structures/voxel/spk_hexa_plane_voxel_shape.hpp>)
- [includes/structures/voxel/spk_slab_voxel_shape.hpp](<includes/structures/voxel/spk_slab_voxel_shape.hpp>)
- [includes/structures/voxel/spk_slope_voxel_shape.hpp](<includes/structures/voxel/spk_slope_voxel_shape.hpp>)
- [includes/structures/voxel/spk_stair_voxel_shape.hpp](<includes/structures/voxel/spk_stair_voxel_shape.hpp>)
- [includes/structures/voxel/spk_voxel_fluid.hpp](<includes/structures/voxel/spk_voxel_fluid.hpp>)
- [includes/structures/voxel/spk_voxel_mesher_occlusion.hpp](<includes/structures/voxel/spk_voxel_mesher_occlusion.hpp>)
- [includes/structures/widget/spk_widget_style.hpp](<includes/structures/widget/spk_widget_style.hpp>)
- [srcs/structures/game_engine/spk_light_3d.cpp](<srcs/structures/game_engine/spk_light_3d.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_draw_arrays_command.cpp](<srcs/structures/graphics/opengl/spk_opengl_draw_arrays_command.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_draw_elements_command.cpp](<srcs/structures/graphics/opengl/spk_opengl_draw_elements_command.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_draw_elements_instanced_command.cpp](<srcs/structures/graphics/opengl/spk_opengl_draw_elements_instanced_command.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_layout_buffer_object.cpp>)
- [srcs/structures/graphics/rendering/command/spk_directional_light_update_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_directional_light_update_render_command.cpp>)
- [srcs/structures/system/device/window/spk_frame.cpp](<srcs/structures/system/device/window/spk_frame.cpp>)
- [srcs/structures/voxel/spk_prefab.cpp](<srcs/structures/voxel/spk_prefab.cpp>)
- [srcs/structures/voxel/spk_voxel_ray_cast.cpp](<srcs/structures/voxel/spk_voxel_ray_cast.cpp>)
- [srcs/structures/voxel/spk_voxel_shape.cpp](<srcs/structures/voxel/spk_voxel_shape.cpp>)

**Wave 6 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 7 — Voxel systems and OpenGL/rendering infrastructure

**32 files**

- [includes/structures/game_engine/spk_entity_2d.hpp](<includes/structures/game_engine/spk_entity_2d.hpp>)
- [includes/structures/game_engine/spk_entity_3d.hpp](<includes/structures/game_engine/spk_entity_3d.hpp>)
- [includes/structures/graphics/geometry/spk_color_mesh_2d.hpp](<includes/structures/graphics/geometry/spk_color_mesh_2d.hpp>)
- [includes/structures/graphics/geometry/spk_mesh_2d.hpp](<includes/structures/graphics/geometry/spk_mesh_2d.hpp>)
- [includes/structures/graphics/geometry/spk_texture_mesh_2d.hpp](<includes/structures/graphics/geometry/spk_texture_mesh_2d.hpp>)
- [includes/structures/graphics/geometry/spk_texture_mesh_3d.hpp](<includes/structures/graphics/geometry/spk_texture_mesh_3d.hpp>)
- [includes/structures/graphics/geometry/spk_voxel_mesh_3d.hpp](<includes/structures/graphics/geometry/spk_voxel_mesh_3d.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_render_context.hpp](<includes/structures/graphics/opengl/spk_opengl_render_context.hpp>)
- [includes/structures/graphics/opengl/spk_opengl_viewport.hpp](<includes/structures/graphics/opengl/spk_opengl_viewport.hpp>)
- [includes/structures/graphics/rendering/command/spk_camera_update_render_command.hpp](<includes/structures/graphics/rendering/command/spk_camera_update_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp](<includes/structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_viewport_render_command.hpp](<includes/structures/graphics/rendering/command/spk_viewport_render_command.hpp>)
- [includes/structures/graphics/rendering/pass/spk_render_target_reference.hpp](<includes/structures/graphics/rendering/pass/spk_render_target_reference.hpp>)
- [includes/structures/graphics/spk_framebuffer_object.hpp](<includes/structures/graphics/spk_framebuffer_object.hpp>)
- [includes/structures/voxel/spk_voxel_fluid_simulator.hpp](<includes/structures/voxel/spk_voxel_fluid_simulator.hpp>)
- [includes/structures/voxel/spk_voxel_registry.hpp](<includes/structures/voxel/spk_voxel_registry.hpp>)
- [srcs/structures/game_engine/spk_camera_3d.cpp](<srcs/structures/game_engine/spk_camera_3d.cpp>)
- [srcs/structures/game_engine/spk_transform_3d.cpp](<srcs/structures/game_engine/spk_transform_3d.cpp>)
- [srcs/structures/graphics/rendering/state/spk_viewport.cpp](<srcs/structures/graphics/rendering/state/spk_viewport.cpp>)
- [srcs/structures/system/win32/spk_winapi_frame.cpp](<srcs/structures/system/win32/spk_winapi_frame.cpp>)
- [srcs/structures/voxel/spk_cross_plane_voxel_shape.cpp](<srcs/structures/voxel/spk_cross_plane_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_cross_voxel_shape.cpp](<srcs/structures/voxel/spk_cross_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_cube_voxel_shape.cpp](<srcs/structures/voxel/spk_cube_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_cuboid_voxel_shape.cpp](<srcs/structures/voxel/spk_cuboid_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_data_voxel_shape.cpp](<srcs/structures/voxel/spk_data_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_hexa_plane_voxel_shape.cpp](<srcs/structures/voxel/spk_hexa_plane_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_slab_voxel_shape.cpp](<srcs/structures/voxel/spk_slab_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_slope_voxel_shape.cpp](<srcs/structures/voxel/spk_slope_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_stair_voxel_shape.cpp](<srcs/structures/voxel/spk_stair_voxel_shape.cpp>)
- [srcs/structures/voxel/spk_voxel_fluid.cpp](<srcs/structures/voxel/spk_voxel_fluid.cpp>)
- [srcs/structures/voxel/spk_voxel_mesher_occlusion.cpp](<srcs/structures/voxel/spk_voxel_mesher_occlusion.cpp>)
- [srcs/structures/widget/spk_widget_style.cpp](<srcs/structures/widget/spk_widget_style.cpp>)

**Wave 7 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 8 — Render units, passes, commands, and scene-facing composition

**28 files**

- [includes/structures/game_engine/rendering/spk_scene_render_frame_request.hpp](<includes/structures/game_engine/rendering/spk_scene_render_frame_request.hpp>)
- [includes/structures/game_engine/spk_sprite_renderer_2d.hpp](<includes/structures/game_engine/spk_sprite_renderer_2d.hpp>)
- [includes/structures/game_engine/spk_texture_mesh_renderer_3d.hpp](<includes/structures/game_engine/spk_texture_mesh_renderer_3d.hpp>)
- [includes/structures/graphics/geometry/spk_mesh.hpp](<includes/structures/graphics/geometry/spk_mesh.hpp>)
- [includes/structures/graphics/geometry/spk_primitive_object.hpp](<includes/structures/graphics/geometry/spk_primitive_object.hpp>)
- [includes/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.hpp](<includes/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_draw_font_render_command.hpp](<includes/structures/graphics/rendering/command/spk_draw_font_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp](<includes/structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp](<includes/structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_draw_texture_mesh_shadow_render_command.hpp](<includes/structures/graphics/rendering/command/spk_draw_texture_mesh_shadow_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.hpp](<includes/structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_draw_voxel_shadow_render_command.hpp](<includes/structures/graphics/rendering/command/spk_draw_voxel_shadow_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_instanced_sprite_render_command.hpp](<includes/structures/graphics/rendering/command/spk_instanced_sprite_render_command.hpp>)
- [includes/structures/graphics/rendering/context/spk_render_context.hpp](<includes/structures/graphics/rendering/context/spk_render_context.hpp>)
- [includes/structures/graphics/rendering/pass/spk_render_pass.hpp](<includes/structures/graphics/rendering/pass/spk_render_pass.hpp>)
- [includes/structures/voxel/spk_voxel_mesher.hpp](<includes/structures/voxel/spk_voxel_mesher.hpp>)
- [srcs/structures/game_engine/spk_camera_2d.cpp](<srcs/structures/game_engine/spk_camera_2d.cpp>)
- [srcs/structures/game_engine/spk_component_2d.cpp](<srcs/structures/game_engine/spk_component_2d.cpp>)
- [srcs/structures/game_engine/spk_component_3d.cpp](<srcs/structures/game_engine/spk_component_3d.cpp>)
- [srcs/structures/game_engine/spk_transform_2d.cpp](<srcs/structures/game_engine/spk_transform_2d.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_object.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_program.cpp](<srcs/structures/graphics/opengl/spk_opengl_program.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_render_context.cpp](<srcs/structures/graphics/opengl/spk_opengl_render_context.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_viewport.cpp](<srcs/structures/graphics/opengl/spk_opengl_viewport.cpp>)
- [srcs/structures/graphics/rendering/command/spk_camera_update_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_camera_update_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_directional_shadow_update_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_directional_shadow_update_render_command.cpp>)
- [srcs/structures/graphics/spk_uniform.cpp](<srcs/structures/graphics/spk_uniform.cpp>)
- [srcs/structures/voxel/spk_voxel_registry.cpp](<srcs/structures/voxel/spk_voxel_registry.cpp>)

**Wave 8 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 9 — Render execution, snapshots, and higher-level graphics composition

**33 files**

- [includes/structures/game_engine/rendering/spk_scene_render_build_context.hpp](<includes/structures/game_engine/rendering/spk_scene_render_build_context.hpp>)
- [includes/structures/game_engine/rendering/spk_shadow_render_pass.hpp](<includes/structures/game_engine/rendering/spk_shadow_render_pass.hpp>)
- [includes/structures/graphics/rendering/command/spk_color_rectangle_render_command.hpp](<includes/structures/graphics/rendering/command/spk_color_rectangle_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_image_render_command.hpp](<includes/structures/graphics/rendering/command/spk_image_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_nine_slice_render_command.hpp](<includes/structures/graphics/rendering/command/spk_nine_slice_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_sprite_render_command.hpp](<includes/structures/graphics/rendering/command/spk_sprite_render_command.hpp>)
- [includes/structures/graphics/rendering/command/spk_text_render_command.hpp](<includes/structures/graphics/rendering/command/spk_text_render_command.hpp>)
- [includes/structures/graphics/rendering/plan/spk_render_plan.hpp](<includes/structures/graphics/rendering/plan/spk_render_plan.hpp>)
- [includes/structures/system/device/runtime/spk_platform_runtime.hpp](<includes/structures/system/device/runtime/spk_platform_runtime.hpp>)
- [includes/structures/voxel/spk_voxel_chunk_renderer.hpp](<includes/structures/voxel/spk_voxel_chunk_renderer.hpp>)
- [srcs/structures/application/module/spk_render_module.cpp](<srcs/structures/application/module/spk_render_module.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_buffer_object.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_clear_command.cpp](<srcs/structures/graphics/opengl/spk_opengl_clear_command.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_framebuffer_object.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_sampler_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_sampler_object.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_uniform_buffer_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_uniform_buffer_object.cpp>)
- [srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp](<srcs/structures/graphics/opengl/spk_opengl_vertex_array_object.cpp>)
- [srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_draw_font_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_draw_font_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_draw_texture_mesh_shadow_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_draw_texture_mesh_shadow_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_draw_voxel_shadow_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_draw_voxel_shadow_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_instanced_sprite_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_viewport_render_command.cpp>)
- [srcs/structures/graphics/rendering/pass/spk_render_pass.cpp](<srcs/structures/graphics/rendering/pass/spk_render_pass.cpp>)
- [srcs/structures/graphics/rendering/snapshot/spk_render_snapshot.cpp](<srcs/structures/graphics/rendering/snapshot/spk_render_snapshot.cpp>)
- [srcs/structures/graphics/rendering/unit/spk_render_unit.cpp](<srcs/structures/graphics/rendering/unit/spk_render_unit.cpp>)
- [srcs/structures/graphics/spk_framebuffer_object.cpp](<srcs/structures/graphics/spk_framebuffer_object.cpp>)
- [srcs/structures/graphics/texture/spk_texture.cpp](<srcs/structures/graphics/texture/spk_texture.cpp>)
- [srcs/structures/voxel/spk_voxel_mesher.cpp](<srcs/structures/voxel/spk_voxel_mesher.cpp>)

**Wave 9 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 10 — Higher-level rendering, platform/window services, and engine types

**14 files**

- [includes/structures/game_engine/rendering/spk_scene_render_pipeline_feature.hpp](<includes/structures/game_engine/rendering/spk_scene_render_pipeline_feature.hpp>)
- [includes/structures/graphics/rendering/pipeline/spk_render_pipeline.hpp](<includes/structures/graphics/rendering/pipeline/spk_render_pipeline.hpp>)
- [includes/structures/system/device/window/spk_window_host.hpp](<includes/structures/system/device/window/spk_window_host.hpp>)
- [includes/structures/system/win32/spk_winapi_platform_runtime.hpp](<includes/structures/system/win32/spk_winapi_platform_runtime.hpp>)
- [includes/structures/voxel/spk_voxel_chunk.hpp](<includes/structures/voxel/spk_voxel_chunk.hpp>)
- [includes/structures/widget/spk_widget.hpp](<includes/structures/widget/spk_widget.hpp>)
- [srcs/structures/graphics/rendering/command/spk_color_rectangle_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_color_rectangle_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_image_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_image_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_nine_slice_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_sprite_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_sprite_render_command.cpp>)
- [srcs/structures/graphics/rendering/command/spk_text_render_command.cpp](<srcs/structures/graphics/rendering/command/spk_text_render_command.cpp>)
- [srcs/structures/graphics/rendering/plan/spk_render_plan.cpp](<srcs/structures/graphics/rendering/plan/spk_render_plan.cpp>)
- [srcs/structures/system/device/runtime/spk_platform_runtime.cpp](<srcs/structures/system/device/runtime/spk_platform_runtime.cpp>)
- [srcs/structures/voxel/spk_voxel_chunk_renderer.cpp](<srcs/structures/voxel/spk_voxel_chunk_renderer.cpp>)

**Wave 10 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 11 — Application modules and widget-rendering foundations

**21 files**

- [includes/structures/application/module/spk_module.hpp](<includes/structures/application/module/spk_module.hpp>)
- [includes/structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp](<includes/structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp>)
- [includes/structures/game_engine/rendering/spk_scene_render_pipeline.hpp](<includes/structures/game_engine/rendering/spk_scene_render_pipeline.hpp>)
- [includes/structures/game_engine/spk_component_logic.hpp](<includes/structures/game_engine/spk_component_logic.hpp>)
- [includes/structures/system/device/window/spk_window_snapshot_manager.hpp](<includes/structures/system/device/window/spk_window_snapshot_manager.hpp>)
- [includes/structures/voxel/spk_voxel_map.hpp](<includes/structures/voxel/spk_voxel_map.hpp>)
- [includes/structures/widget/spk_animation_label.hpp](<includes/structures/widget/spk_animation_label.hpp>)
- [includes/structures/widget/spk_container_widget.hpp](<includes/structures/widget/spk_container_widget.hpp>)
- [includes/structures/widget/spk_image_label.hpp](<includes/structures/widget/spk_image_label.hpp>)
- [includes/structures/widget/spk_layout.hpp](<includes/structures/widget/spk_layout.hpp>)
- [includes/structures/widget/spk_panel.hpp](<includes/structures/widget/spk_panel.hpp>)
- [includes/structures/widget/spk_scalable_widget.hpp](<includes/structures/widget/spk_scalable_widget.hpp>)
- [includes/structures/widget/spk_screen.hpp](<includes/structures/widget/spk_screen.hpp>)
- [includes/structures/widget/spk_spacer_widget.hpp](<includes/structures/widget/spk_spacer_widget.hpp>)
- [includes/structures/widget/spk_text_area.hpp](<includes/structures/widget/spk_text_area.hpp>)
- [includes/structures/widget/spk_text_edit.hpp](<includes/structures/widget/spk_text_edit.hpp>)
- [includes/structures/widget/spk_text_label.hpp](<includes/structures/widget/spk_text_label.hpp>)
- [srcs/structures/graphics/rendering/pipeline/spk_render_pipeline.cpp](<srcs/structures/graphics/rendering/pipeline/spk_render_pipeline.cpp>)
- [srcs/structures/system/device/window/spk_window_host.cpp](<srcs/structures/system/device/window/spk_window_host.cpp>)
- [srcs/structures/system/win32/spk_winapi_platform_runtime.cpp](<srcs/structures/system/win32/spk_winapi_platform_runtime.cpp>)
- [srcs/structures/widget/spk_widget.cpp](<srcs/structures/widget/spk_widget.cpp>)

**Wave 11 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 12 — Widget hierarchy and application/game integration

**36 files**

- [includes/structures/application/module/spk_frame_module.hpp](<includes/structures/application/module/spk_frame_module.hpp>)
- [includes/structures/application/module/spk_keyboard_module.hpp](<includes/structures/application/module/spk_keyboard_module.hpp>)
- [includes/structures/application/module/spk_mouse_module.hpp](<includes/structures/application/module/spk_mouse_module.hpp>)
- [includes/structures/application/module/spk_update_module.hpp](<includes/structures/application/module/spk_update_module.hpp>)
- [includes/structures/game_engine/spk_animation_logic.hpp](<includes/structures/game_engine/spk_animation_logic.hpp>)
- [includes/structures/game_engine/spk_component_logic_registry.hpp](<includes/structures/game_engine/spk_component_logic_registry.hpp>)
- [includes/structures/game_engine/spk_sprite_render_logic.hpp](<includes/structures/game_engine/spk_sprite_render_logic.hpp>)
- [includes/structures/game_engine/spk_texture_mesh_render_logic.hpp](<includes/structures/game_engine/spk_texture_mesh_render_logic.hpp>)
- [includes/structures/voxel/spk_voxel_chunk_render_logic.hpp](<includes/structures/voxel/spk_voxel_chunk_render_logic.hpp>)
- [includes/structures/voxel/spk_voxel_chunk_streamer_logic.hpp](<includes/structures/voxel/spk_voxel_chunk_streamer_logic.hpp>)
- [includes/structures/voxel/spk_voxel_chunk_transparent_render_logic.hpp](<includes/structures/voxel/spk_voxel_chunk_transparent_render_logic.hpp>)
- [includes/structures/voxel/spk_voxel_fluid_simulation_logic.hpp](<includes/structures/voxel/spk_voxel_fluid_simulation_logic.hpp>)
- [includes/structures/widget/spk_dynamic_text_label.hpp](<includes/structures/widget/spk_dynamic_text_label.hpp>)
- [includes/structures/widget/spk_form_layout.hpp](<includes/structures/widget/spk_form_layout.hpp>)
- [includes/structures/widget/spk_grid_layout.hpp](<includes/structures/widget/spk_grid_layout.hpp>)
- [includes/structures/widget/spk_linear_layout.hpp](<includes/structures/widget/spk_linear_layout.hpp>)
- [includes/structures/widget/spk_push_button.hpp](<includes/structures/widget/spk_push_button.hpp>)
- [includes/structures/widget/spk_slider_bar.hpp](<includes/structures/widget/spk_slider_bar.hpp>)
- [srcs/structures/application/module/spk_window_module.cpp](<srcs/structures/application/module/spk_window_module.cpp>)
- [srcs/structures/game_engine/rendering/spk_scene_lighting_render_feature.cpp](<srcs/structures/game_engine/rendering/spk_scene_lighting_render_feature.cpp>)
- [srcs/structures/game_engine/spk_component_logic.cpp](<srcs/structures/game_engine/spk_component_logic.cpp>)
- [srcs/structures/system/device/window/spk_window_snapshot_manager.cpp](<srcs/structures/system/device/window/spk_window_snapshot_manager.cpp>)
- [srcs/structures/voxel/spk_voxel_chunk.cpp](<srcs/structures/voxel/spk_voxel_chunk.cpp>)
- [srcs/structures/voxel/spk_voxel_chunk_streamer.cpp](<srcs/structures/voxel/spk_voxel_chunk_streamer.cpp>)
- [srcs/structures/voxel/spk_voxel_fluid_simulator.cpp](<srcs/structures/voxel/spk_voxel_fluid_simulator.cpp>)
- [srcs/structures/widget/spk_animation_label.cpp](<srcs/structures/widget/spk_animation_label.cpp>)
- [srcs/structures/widget/spk_container_widget.cpp](<srcs/structures/widget/spk_container_widget.cpp>)
- [srcs/structures/widget/spk_image_label.cpp](<srcs/structures/widget/spk_image_label.cpp>)
- [srcs/structures/widget/spk_layout.cpp](<srcs/structures/widget/spk_layout.cpp>)
- [srcs/structures/widget/spk_panel.cpp](<srcs/structures/widget/spk_panel.cpp>)
- [srcs/structures/widget/spk_scalable_widget.cpp](<srcs/structures/widget/spk_scalable_widget.cpp>)
- [srcs/structures/widget/spk_screen.cpp](<srcs/structures/widget/spk_screen.cpp>)
- [srcs/structures/widget/spk_spacer_widget.cpp](<srcs/structures/widget/spk_spacer_widget.cpp>)
- [srcs/structures/widget/spk_text_area.cpp](<srcs/structures/widget/spk_text_area.cpp>)
- [srcs/structures/widget/spk_text_edit.cpp](<srcs/structures/widget/spk_text_edit.cpp>)
- [srcs/structures/widget/spk_text_label.cpp](<srcs/structures/widget/spk_text_label.cpp>)

**Wave 12 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 13 — Composite widgets, engine integration, and application integration

**23 files**

- [includes/structures/game_engine/spk_game_engine.hpp](<includes/structures/game_engine/spk_game_engine.hpp>)
- [includes/structures/system/device/window/spk_window.hpp](<includes/structures/system/device/window/spk_window.hpp>)
- [includes/structures/widget/spk_action_bar.hpp](<includes/structures/widget/spk_action_bar.hpp>)
- [includes/structures/widget/spk_command_panel.hpp](<includes/structures/widget/spk_command_panel.hpp>)
- [includes/structures/widget/spk_debug_overlay.hpp](<includes/structures/widget/spk_debug_overlay.hpp>)
- [includes/structures/widget/spk_icon_button.hpp](<includes/structures/widget/spk_icon_button.hpp>)
- [includes/structures/widget/spk_interface_window.hpp](<includes/structures/widget/spk_interface_window.hpp>)
- [includes/structures/widget/spk_scroll_bar.hpp](<includes/structures/widget/spk_scroll_bar.hpp>)
- [includes/structures/widget/spk_spin_box.hpp](<includes/structures/widget/spk_spin_box.hpp>)
- [srcs/structures/application/module/spk_frame_module.cpp](<srcs/structures/application/module/spk_frame_module.cpp>)
- [srcs/structures/application/module/spk_keyboard_module.cpp](<srcs/structures/application/module/spk_keyboard_module.cpp>)
- [srcs/structures/application/module/spk_mouse_module.cpp](<srcs/structures/application/module/spk_mouse_module.cpp>)
- [srcs/structures/application/module/spk_update_module.cpp](<srcs/structures/application/module/spk_update_module.cpp>)
- [srcs/structures/game_engine/rendering/spk_scene_render_pipeline.cpp](<srcs/structures/game_engine/rendering/spk_scene_render_pipeline.cpp>)
- [srcs/structures/game_engine/spk_component_logic_registry.cpp](<srcs/structures/game_engine/spk_component_logic_registry.cpp>)
- [srcs/structures/voxel/spk_voxel_chunk_render_logic.cpp](<srcs/structures/voxel/spk_voxel_chunk_render_logic.cpp>)
- [srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp](<srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp>)
- [srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp](<srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp>)
- [srcs/structures/voxel/spk_voxel_fluid_simulation_logic.cpp](<srcs/structures/voxel/spk_voxel_fluid_simulation_logic.cpp>)
- [srcs/structures/widget/spk_dynamic_text_label.cpp](<srcs/structures/widget/spk_dynamic_text_label.cpp>)
- [srcs/structures/widget/spk_form_layout.cpp](<srcs/structures/widget/spk_form_layout.cpp>)
- [srcs/structures/widget/spk_push_button.cpp](<srcs/structures/widget/spk_push_button.cpp>)
- [srcs/structures/widget/spk_slider_bar.cpp](<srcs/structures/widget/spk_slider_bar.cpp>)

**Wave 13 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 14 — Final widget/window/game integration

**18 files**

- [includes/structures/system/device/window/spk_window_registry.hpp](<includes/structures/system/device/window/spk_window_registry.hpp>)
- [includes/structures/widget/spk_checkable_icon_button.hpp](<includes/structures/widget/spk_checkable_icon_button.hpp>)
- [includes/structures/widget/spk_game_engine_widget.hpp](<includes/structures/widget/spk_game_engine_widget.hpp>)
- [includes/structures/widget/spk_message_box.hpp](<includes/structures/widget/spk_message_box.hpp>)
- [includes/structures/widget/spk_numeric_spin_box.hpp](<includes/structures/widget/spk_numeric_spin_box.hpp>)
- [includes/structures/widget/spk_prompt_panel.hpp](<includes/structures/widget/spk_prompt_panel.hpp>)
- [includes/structures/widget/spk_scroll_area.hpp](<includes/structures/widget/spk_scroll_area.hpp>)
- [includes/structures/widget/spk_workspace.hpp](<includes/structures/widget/spk_workspace.hpp>)
- [srcs/structures/game_engine/spk_game_engine.cpp](<srcs/structures/game_engine/spk_game_engine.cpp>)
- [srcs/structures/system/device/window/spk_window.cpp](<srcs/structures/system/device/window/spk_window.cpp>)
- [srcs/structures/system/device/window/spk_window_handle.cpp](<srcs/structures/system/device/window/spk_window_handle.cpp>)
- [srcs/structures/voxel/spk_voxel_map.cpp](<srcs/structures/voxel/spk_voxel_map.cpp>)
- [srcs/structures/widget/spk_action_bar.cpp](<srcs/structures/widget/spk_action_bar.cpp>)
- [srcs/structures/widget/spk_command_panel.cpp](<srcs/structures/widget/spk_command_panel.cpp>)
- [srcs/structures/widget/spk_debug_overlay.cpp](<srcs/structures/widget/spk_debug_overlay.cpp>)
- [srcs/structures/widget/spk_icon_button.cpp](<srcs/structures/widget/spk_icon_button.cpp>)
- [srcs/structures/widget/spk_interface_window.cpp](<srcs/structures/widget/spk_interface_window.cpp>)
- [srcs/structures/widget/spk_scroll_bar.cpp](<srcs/structures/widget/spk_scroll_bar.cpp>)

**Wave 14 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 15 — Top-level application, widget, and window wrappers

**7 files**

- [includes/structures/application/spk_application.hpp](<includes/structures/application/spk_application.hpp>)
- [srcs/structures/system/device/window/spk_window_registry.cpp](<srcs/structures/system/device/window/spk_window_registry.cpp>)
- [srcs/structures/widget/spk_checkable_icon_button.cpp](<srcs/structures/widget/spk_checkable_icon_button.cpp>)
- [srcs/structures/widget/spk_game_engine_widget.cpp](<srcs/structures/widget/spk_game_engine_widget.cpp>)
- [srcs/structures/widget/spk_message_box.cpp](<srcs/structures/widget/spk_message_box.cpp>)
- [srcs/structures/widget/spk_prompt_panel.cpp](<srcs/structures/widget/spk_prompt_panel.cpp>)
- [srcs/structures/widget/spk_scroll_area.cpp](<srcs/structures/widget/spk_scroll_area.cpp>)

**Wave 15 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 16 — Public umbrella header and final application implementation

**2 files**

- [includes/sparkle.hpp](<includes/sparkle.hpp>)
- [srcs/structures/application/spk_application.cpp](<srcs/structures/application/spk_application.cpp>)

**Wave 16 exit check:** [ ] Every file above has been read [ ] Remarks recorded [ ] Relevant tests checked [ ] Follow-up fixes grouped

### Wave 17 — CMake forwarding include (`includes/sparkle`)

**1 file**

- [includes/sparkle](<includes/sparkle>)

**Wave 17 exit check:** [ ] The public forwarding/install entry point matches the intended public API [ ] The umbrella include remains buildable

## Final review pass

After Wave 17, review the library as a whole:

- [ ] Every implementation file is marked reviewed in the files indexed by [library_review_notes/README.md](<library_review_notes/README.md>).
- [ ] Every public type has one clear responsibility and one clear owner for mutable state.
- [ ] Header/source boundaries are intentional; accidental header-only or source-only implementations are identified.
- [ ] Include direction is understandable and cycles introduced by future changes are rejected.
- [ ] Ownership, lifetime, thread-safety, error, and invalid-input rules are written down before fixes are implemented.
- [ ] Tests cover the invariants discovered during the review, not only the current happy paths.
- [ ] Fixes are applied in dependency order, then the full test suite and visual/performance checks are rerun.

## Important limitation

The ordering is based on textual includes. It cannot discover dependencies hidden behind forward declarations, templates instantiated elsewhere, macros, runtime registration, linker behavior, resource lookups, or semantic coupling. Those are specifically things to look for while reviewing each file.
