#pragma once
#include "augs/math/vec2.h"

#include "game/transcendental/entity_id.h"
#include "game/transcendental/entity_handle_declaration.h"

#include "augs/misc/timer.h"
#include "augs/graphics/pixel.h"
#include "padding_byte.h"

namespace components {
	struct damage {
		// GEN INTROSPECTOR struct components::damage
		float amount = 12.f;

		float impulse_upon_hit = 100.f;

		entity_id sender;

		bool damage_upon_collision = true;
		bool destroy_upon_damage = true;
		bool constrain_lifetime = true;
		bool constrain_distance = false;

		int damage_charges_before_destruction = 1;

		vec2 custom_impact_velocity;

		bool damage_falloff = false;
		std::array<padding_byte, 3> pad;

		float damage_falloff_starting_distance = 500.f;
		float minimum_amount_after_falloff = 5.f;

		float distance_travelled = 0.f;
		float max_distance = 0.f;
		float max_lifetime_ms = 2000.f;
		float recoil_multiplier = 1.f;

		float lifetime_ms = 0.f;

		float homing_towards_hostile_strength = 0.f;
		entity_id particular_homing_target;

		vec2 saved_point_of_impact_before_death;
		// END GEN INTROSPECTOR
	};
}