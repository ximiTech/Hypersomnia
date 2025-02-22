#pragma once
#include "augs/math/vec2.h"
#include "game/components/transform_component.h"
#include "augs/misc/smooth_value_field.h"
#include "augs/math/camera_cone.h"
#include "game/cosmos/entity_handle_declaration.h"
#include "game/modes/detail/fog_of_war_settings.h"

namespace augs {
	class delta;
}

class interpolation_system;

struct world_camera_settings {
	// GEN INTROSPECTOR struct world_camera_settings
	augs::smoothing_settings<double> additional_position_smoothing;
	augs::smoothing_settings<float> smoothing;

	float angled_look_length = 100.f;
	float look_bound_expand = 0.5f;
	bool enable_smoothing = true;
	bool adjust_zoom_to_available_fog_of_war_size = true;
	int adjust_zoom_if_fov_bigger_by_pixels = 10;
	// END GEN INTROSPECTOR
};

struct world_camera {
	bool dont_smooth_once = false;

	transformr last_interpolant;

	vec2 player_position_previously_seen;
	vec2 player_position_at_previous_step;

	augs::smooth_value_field additional_position_smoothing;

	camera_eye get_current_eye() const;

	void tick(
		const vec2i screen_size,
		const fog_of_war_settings& fog_of_war,
		const interpolation_system& interp, 
		augs::delta dt,
		world_camera_settings settings,
		const_entity_handle entity_to_chase,
		const vec2 crosshair_displacement
	);

	auto get_effective_flash_mult() const {
		return last_registered_flash_mult;
	}

	bool is_flash_afterimage_requested() const {
		return request_afterimage;
	}

private:
	camera_eye current_eye;

	void advance_flash(const_entity_handle viewer, augs::delta dt);
	float after_flash_passed_ms = 0.f;
	float last_registered_flash_mult = 0.f;
	bool request_afterimage = false;

	vec2 get_camera_offset_due_to_character_crosshair(
		const const_entity_handle,
		const world_camera_settings,
		const vec2 screen_size,
		const vec2 crosshair_displacement
	) const;

};