#pragma once

// Traits & utilities
#include "spk_activable_trait.hpp"
#include "spk_binary_field.hpp"
#include "spk_blockable_trait.hpp"
#include "spk_cached_data.hpp"
#include "spk_concepts.hpp"
#include "spk_constants.hpp"
#include "spk_contract_provider.hpp"
#include "spk_flags.hpp"
#include "spk_inherence_trait.hpp"
#include "spk_object_pool.hpp"
#include "spk_observable_value.hpp"
#include "spk_synchronizable_trait.hpp"
#include "spk_thread_safe_contract.hpp"
#include "spk_thread_safe_deque.hpp"

// Math & geometry
#include "spk_approx_value.hpp"
#include "spk_rect_2d.hpp"
#include "spk_vector2.hpp"

// Time
#include "spk_chronometer.hpp"
#include "spk_duration.hpp"
#include "spk_timer.hpp"
#include "spk_timestamp.hpp"
#include "spk_time_unit.hpp"
#include "spk_time_utils.hpp"
#include "spk_update_tick.hpp"

// Input
#include "spk_input_state.hpp"
#include "spk_keyboard.hpp"
#include "spk_mouse.hpp"
#include "spk_events.hpp"

// Platform / GPU runtime interfaces
#include "spk_surface_state.hpp"
#include "spk_frame.hpp"
#include "spk_platform_runtime.hpp"
#include "spk_render_context.hpp"
#include "spk_gpu_platform_runtime.hpp"
#ifdef _WIN32
#include "spk_winapi_class.hpp"
#include "spk_winapi_cursor.hpp"
#include "spk_winapi_window.hpp"
#include "spk_winapi_frame.hpp"
#include "spk_winapi_platform_runtime.hpp"
#include "spk_opengl_render_context.hpp"
#include "spk_opengl_runtime.hpp"
#endif

// Rendering
#include "spk_render_command.hpp"
#include "spk_render_unit.hpp"
#include "spk_render_unit_builder.hpp"
#include "spk_render_snapshot.hpp"
#include "spk_render_snapshot_builder.hpp"
#include "spk_render_module.hpp"
#ifdef SPARKLE_GPU_BACKEND_OPENGL
#include "spk_program.hpp"
#include "spk_opengl_bind_vertex_array_command.hpp"
#include "spk_opengl_buffer_object.hpp"
#include "spk_opengl_clear_command.hpp"
#include "spk_opengl_draw_arrays_command.hpp"
#include "spk_opengl_draw_elements_command.hpp"
#include "spk_opengl_draw_elements_instanced_command.hpp"
#include "spk_opengl_index_buffer_object.hpp"
#include "spk_opengl_primitive.hpp"
#include "spk_opengl_scissor_command.hpp"
#include "spk_opengl_shader_storage_buffer_object.hpp"
#include "spk_opengl_uniform_buffer_object.hpp"
#include "spk_opengl_use_program_render_command.hpp"
#include "spk_opengl_vertex_array_object.hpp"
#include "spk_opengl_vertex_buffer_object.hpp"
#include "spk_opengl_viewport_command.hpp"
#endif

// Modules
#include "spk_module.hpp"
#include "spk_frame_module.hpp"
#include "spk_keyboard_module.hpp"
#include "spk_mouse_module.hpp"
#include "spk_update_module.hpp"

// Widget system
#include "spk_widget.hpp"

// Window & application
#include "spk_window_host.hpp"
#include "spk_window_handle.hpp"
#include "spk_window.hpp"
#include "spk_window_registry.hpp"
#include "spk_application.hpp"
