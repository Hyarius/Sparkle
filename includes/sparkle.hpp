#pragma once

// Traits & utilities
#include "traits/spk_activable_trait.hpp"
#include "utils/spk_binary_field.hpp"
#include "traits/spk_blockable_trait.hpp"
#include "utils/spk_cached_data.hpp"
#include "spk_concepts.hpp"
#include "math/spk_constants.hpp"
#include "utils/spk_contract_provider.hpp"
#include "rendering/spk_color.hpp"
#include "utils/spk_flags.hpp"
#include "traits/spk_inherence_trait.hpp"
#include "utils/spk_object_pool.hpp"
#include "utils/spk_observable_value.hpp"
#include "traits/spk_synchronizable_trait.hpp"
#include "utils/spk_thread_safe_contract.hpp"
#include "utils/spk_thread_safe_deque.hpp"

// Math & geometry
#include "math/spk_approx_value.hpp"
#include "math/spk_math.hpp"
#include "math/spk_rect_2d.hpp"
#include "math/spk_matrix.hpp"
#include "rendering/spk_mesh.hpp"
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"
#include "math/spk_vector4.hpp"
#include "math/spk_quaternion.hpp"

// Time
#include "time/spk_chronometer.hpp"
#include "time/spk_duration.hpp"
#include "time/spk_timer.hpp"
#include "time/spk_timestamp.hpp"
#include "time/spk_time_unit.hpp"
#include "time/spk_time_utils.hpp"
#include "time/spk_update_tick.hpp"

// Input
#include "input/spk_input_state.hpp"
#include "input/spk_keyboard.hpp"
#include "input/spk_mouse.hpp"
#include "input/spk_events.hpp"

// Platform / GPU runtime interfaces
#include "window/spk_surface_state.hpp"
#include "window/spk_frame.hpp"
#include "application/spk_platform_runtime.hpp"
#include "rendering/spk_render_context.hpp"
#include "opengl/spk_gpu_platform_runtime.hpp"
#ifdef _WIN32
#include "winapi/spk_winapi_class.hpp"
#include "winapi/spk_winapi_cursor.hpp"
#include "winapi/spk_winapi_window.hpp"
#include "winapi/spk_winapi_frame.hpp"
#include "winapi/spk_winapi_platform_runtime.hpp"
#include "opengl/spk_opengl_render_context.hpp"
#include "opengl/spk_opengl_runtime.hpp"
#endif

// Rendering
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_draw_color_mesh_render_command.hpp"
#include "rendering/spk_render_unit.hpp"
#include "rendering/spk_render_unit_builder.hpp"
#include "rendering/spk_render_snapshot.hpp"
#include "rendering/spk_render_snapshot_builder.hpp"
#include "application/module/spk_render_module.hpp"
#include "rendering/spk_generic_mesh.hpp"
#include "rendering/spk_mesh_2d.hpp"
#include "rendering/spk_color_mesh_2d.hpp"
#ifdef SPARKLE_GPU_BACKEND_OPENGL
#include "opengl/spk_opengl_program.hpp"
#include "opengl/spk_opengl_buffer_object.hpp"
#include "opengl/spk_opengl_clear_command.hpp"
#include "opengl/spk_opengl_draw_arrays_command.hpp"
#include "opengl/spk_opengl_draw_elements_command.hpp"
#include "opengl/spk_opengl_draw_elements_instanced_command.hpp"
#include "opengl/spk_opengl_index_buffer_object.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_primitive.hpp"
#include "opengl/spk_opengl_scissor_command.hpp"
#include "opengl/spk_opengl_shader_storage_buffer_object.hpp"
#include "opengl/spk_opengl_uniform.hpp"
#include "opengl/spk_opengl_uniform_buffer_object.hpp"
#include "opengl/spk_opengl_vertex_array_object.hpp"
#include "opengl/spk_opengl_vertex_buffer_object.hpp"
#include "opengl/spk_opengl_viewport_command.hpp"
#endif

// Modules
#include "application/module/spk_module.hpp"
#include "application/module/spk_frame_module.hpp"
#include "application/module/spk_keyboard_module.hpp"
#include "application/module/spk_mouse_module.hpp"
#include "application/module/spk_update_module.hpp"

// Widget system
#include "widget/spk_widget.hpp"

// Window & application
#include "window/spk_window_host.hpp"
#include "window/spk_window_handle.hpp"
#include "window/spk_window.hpp"
#include "window/spk_window_registry.hpp"
#include "application/spk_application.hpp"
