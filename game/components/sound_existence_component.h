#pragma once
#include "augs/misc/stepped_timing.h"
#include "game/assets/sound_buffer_id.h"

#include "augs/padding_byte.h"
#include "game/transcendental/entity_id.h"
#include "game/transcendental/entity_handle_declaration.h"
#include "augs/audio/sound_effect_modifier.h"

struct sound_effect_input {
	// GEN INTROSPECTOR struct sound_effect_input
	sound_response effect;
	bool delete_entity_after_effect_lifetime = true;
	char variation_number = -1;
	std::array<padding_byte, 2> pad;
	entity_id direct_listener;
	// END GEN INTROSPECTOR

	entity_handle create_sound_effect_entity(
		cosmos& cosmos,
		const components::transform place_of_birth,
		const entity_id chased_subject_id
	) const;
};

namespace components {
	struct sound_existence {
		// GEN INTROSPECTOR struct components::sound_existence
		sound_effect_input input;

		augs::stepped_timestamp time_of_birth;
		unsigned max_lifetime_in_steps = 0u;
		// END GEN INTROSPECTOR

		static bool is_activated(const const_entity_handle);
		static void activate(const entity_handle);
		static void deactivate(const entity_handle);

		float calculate_max_audible_distance() const;

		size_t random_variation_number_from_transform(const components::transform) const;
	};
}