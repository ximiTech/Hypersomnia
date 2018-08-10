#include "game/cosmos/cosmos.h"
#include "game/assets/all_logical_assets.h"
#include "game/components/rigid_body_component.h"
#include "game/components/fixtures_component.h"
#include "game/components/shape_polygon_component.h"
#include "game/components/sprite_component.h"

#include "game/enums/filters.h"
#include "game/cosmos/entity_handle.h"

namespace test_flavours {
	template <class E>
	void add_explosion_body(E& meta) {
		invariants::fixtures fixtures_def;
		invariants::rigid_body body_def;

		body_def.damping.linear = 0.5f;
		body_def.damping.angular = 0.f;
		body_def.bullet = true;

		fixtures_def.filter = filters::pathfinding_query();
		fixtures_def.density = 1;
		fixtures_def.material = to_physical_material_id(test_scene_physical_material_id::METAL);
		fixtures_def.restitution = 1.2f;

		meta.set(fixtures_def);
		meta.set(body_def);
	}

	template <class E>
	void add_standard_dynamic_body(E& meta) {
		invariants::fixtures fixtures_def;
		invariants::rigid_body body_def;

		body_def.damping.linear = 6.5f;
		body_def.damping.angular = 6.5f;

		fixtures_def.filter = filters::wall();
		fixtures_def.density = 1;
		fixtures_def.material = to_physical_material_id(test_scene_physical_material_id::METAL);

		meta.set(fixtures_def);
		meta.set(body_def);
	}

	template <class E>
	auto& add_lying_item_dynamic_body(E& meta) {
		invariants::fixtures fixtures_def;
		invariants::rigid_body body_def;

		body_def.damping.linear = 6.5f;
		body_def.damping.angular = 6.5f;

		fixtures_def.filter = filters::lying_item();
		fixtures_def.density = .2f;
		fixtures_def.restitution = .5f;
		fixtures_def.material = to_physical_material_id(test_scene_physical_material_id::METAL);

		meta.set(fixtures_def);
		meta.set(body_def);

		return meta.template get<invariants::fixtures>();
	}

	template <class E>
	void add_shell_dynamic_body(E& meta) {
		invariants::fixtures fixtures_def;
		invariants::rigid_body body_def;

		body_def.damping.linear = 6.5f;
		body_def.damping.angular = 6.5f;

		fixtures_def.filter = filters::shell();
		fixtures_def.restitution = 1.4f;
		fixtures_def.density = 0.001f;
		fixtures_def.collision_sound_gain_mult = 100.f;
		fixtures_def.material = to_physical_material_id(test_scene_physical_material_id::METAL);

		meta.set(fixtures_def);
		meta.set(body_def);
	}

	template <class E>
	void add_remnant_dynamic_body(E& meta) {
		invariants::fixtures fixtures_def;
		invariants::rigid_body body_def;

		body_def.damping.linear = 6.5f;
		body_def.damping.angular = 6.5f;

		fixtures_def.filter = filters::shell();
		fixtures_def.restitution = 1.8f;
		fixtures_def.density = 0.0001f;
		fixtures_def.collision_sound_gain_mult = 1000.f;
		fixtures_def.material = to_physical_material_id(test_scene_physical_material_id::METAL);

		meta.set(fixtures_def);
		meta.set(body_def);
	}

	template <class E>
	void add_standard_static_body(E& meta) {
		invariants::fixtures fixtures_def;
		invariants::rigid_body body_def;

		body_def.damping.linear = 6.5f;
		body_def.damping.angular = 6.5f;

		body_def.body_type = rigid_body_type::STATIC;

		fixtures_def.filter = filters::wall();
		fixtures_def.density = 1;
		fixtures_def.material = to_physical_material_id(test_scene_physical_material_id::METAL);

		meta.set(fixtures_def);
		meta.set(body_def);
	}
	
	template <class E>
	void add_bullet_round_physics(E& meta) {
		invariants::fixtures fixtures_def;
		invariants::rigid_body body_def;

		body_def.bullet = true;
		body_def.damping.angular = 0.f,
		body_def.damping.linear = 0.f,
		body_def.angled_damping = false;
		
		fixtures_def.filter = filters::flying();
		fixtures_def.density = 1;
		fixtures_def.material = to_physical_material_id(test_scene_physical_material_id::METAL);

		meta.set(fixtures_def);
		meta.set(body_def);
	}
}