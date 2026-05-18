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

// Rendering
#include "spk_render_command.hpp"
#include "spk_render_unit.hpp"
#include "spk_render_unit_builder.hpp"
#include "spk_render_snapshot.hpp"
#include "spk_render_snapshot_builder.hpp"
#include "spk_render_module.hpp"

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
