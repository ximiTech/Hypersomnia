#include "ingredients.h"

#include "game/systems_stateless/particles_existence_system.h"
#include "game/systems_stateless/sound_existence_system.h"
#include "game/components/position_copying_component.h"

#include "game/components/crosshair_component.h"
#include "game/components/sprite_component.h"
#include "game/components/movement_component.h"
#include "game/components/rotation_copying_component.h"
#include "game/components/animation_component.h"
#include "game/components/rigid_body_component.h"
#include "game/components/car_component.h"
#include "game/components/trigger_component.h"
#include "game/components/rigid_body_component.h"
#include "game/components/fixtures_component.h"
#include "game/components/special_physics_component.h"
#include "game/components/name_component.h"
#include "game/components/particles_existence_component.h"
#include "game/components/sound_existence_component.h"
#include "game/components/polygon_component.h"
#include "game/transcendental/cosmos.h"

#include "game/assets/assets_manager.h"

#include "game/enums/filters.h"

namespace prefabs {
	entity_handle create_car(const logic_step step, const components::transform& spawn_transform) {
		auto& world = step.cosm;
		const auto& metas = step.input.metas_of_assets;

		auto front = world.create_entity("front");
		auto interior = world.create_entity("interior");
		auto left_wheel = world.create_entity("left_wheel");
		left_wheel.make_as_child_of(front);

		const auto si = world.get_si();

		name_entity(front, entity_name::TRUCK);

		const vec2 front_size = metas[assets::game_image_id::TRUCK_FRONT].get_size();
		const vec2 interior_size = metas[assets::game_image_id::TRUCK_INSIDE].get_size();

		{
			//auto& sprite = front += components::sprite();
			auto& poly = front += components::polygon();
			auto& render = front += components::render();
			auto& car = front += components::car();
			components::rigid_body physics_definition(si, spawn_transform);
			components::fixtures colliders;

			car.interior = interior;
			car.left_wheel_trigger = left_wheel;
			car.input_acceleration.set(2500, 4500) /= 3;
			//car.acceleration_length = 4500 / 5.0;
			car.acceleration_length = 4500 / 6.2f;
			car.speed_for_pitch_unit = 2000.f;

			poly.add_vertices_from(metas[assets::game_image_id::TRUCK_FRONT].shape);
			poly.texture_map = assets::game_image_id::TRUCK_FRONT;
			//sprite.set(assets::game_image_id::TRUCK_FRONT, world);
			//sprite.get_size(metas).x = 200;
			//sprite.get_size(metas).y = 100;

			render.layer = render_layer::DYNAMIC_BODY;

			physics_definition.linear_damping = 0.4f;
			physics_definition.angular_damping = 2.f;

			front.create_fixtures_component_from_renderable(
				step,
				[&](auto component){
					auto& group = component.get_fixture_group_data();

					group.filter = filters::dynamic_object();
					group.density = 0.6;

					front += component;
				}
			);

			front += physics_definition;
			front.get<components::fixtures>().set_owner_body(front);
			//rigid_body.air_resistance = 0.2f;
		}
		
		{
			auto& sprite = interior += components::sprite();
			auto& render = interior += components::render();
			components::fixtures colliders;

			render.layer = render_layer::CAR_INTERIOR;

			sprite.set(assets::game_image_id::TRUCK_INSIDE);
			//sprite.set(assets::game_image_id::TRUCK_INSIDE, rgba(122, 0, 122, 255));
			//sprite.get_size(metas).x = 250;
			//sprite.get_size(metas).y = 550;
			
			vec2 offset((front_size.x / 2 + sprite.get_size(metas).x / 2) * -1, 0);

			interior.create_fixtures_component_from_renderable(
				step,
				[&](auto component){
					auto& group = component.get_fixture_group_data();

					group.filter = filters::friction_ground();
					group.density = 0.6;
					group.offsets_for_created_shapes[colliders_offset_type::SHAPE_OFFSET].pos = offset;
					group.is_friction_ground = true;

					interior += component;
				}
			);

			interior.get<components::fixtures>().set_owner_body(front);
			interior.make_as_child_of(front);
		}

		{
			auto& sprite = left_wheel += components::sprite();
			auto& render = left_wheel += components::render();
			auto& trigger = left_wheel += components::trigger();
			components::fixtures colliders;

			trigger.entity_to_be_notified = front;
			trigger.react_to_collision_detectors = false;
			trigger.react_to_query_detectors = true;

			render.layer = render_layer::CAR_WHEEL;

			sprite.set(assets::game_image_id::CAR_INSIDE, vec2(60, 30), rgba(29, 0, 0, 0));

			auto& fixture = colliders.get_fixture_group_data();

			vec2 offset((front_size.x / 2 + sprite.get_size(metas).x / 2 + 20) * -1, 0);

			left_wheel.create_fixtures_component_from_renderable(
				step,
				[&](auto component){
					auto& group = component.get_fixture_group_data();

					group.filter = filters::trigger();
					group.density = 0.6;
					group.sensor = true;
					group.offsets_for_created_shapes[colliders_offset_type::SHAPE_OFFSET].pos = offset;

					left_wheel += component;
				}
			);

			left_wheel.get<components::fixtures>().set_owner_body(front);
		}

		{
			for (int i = 0; i < 4; ++i) {
				components::transform this_engine_transform;
				const auto engine_physical = world.create_entity("engine_body");
				engine_physical.make_as_child_of(front);

				{

					auto& sprite = engine_physical += components::sprite();
					auto& render = engine_physical += components::render();

					render.layer = render_layer::SMALL_DYNAMIC_BODY;

					sprite.set(assets::game_image_id::TRUCK_ENGINE);
					//sprite.set(assets::game_image_id::TRUCK_INSIDE, rgba(122, 0, 122, 255));
					//sprite.get_size(metas).x = 250;
					//sprite.get_size(metas).y = 550;

					components::transform offset;

					if (i == 0) {
						offset.pos.set((front_size.x / 2 + interior_size.x + sprite.get_size(metas).x / 2 - 5.f) * -1, (-interior_size.y / 2 + sprite.get_size(metas).y / 2));
					}
					if (i == 1) {
						offset.pos.set((front_size.x / 2 + interior_size.x + sprite.get_size(metas).x / 2 - 5.f) * -1, -(-interior_size.y / 2 + sprite.get_size(metas).y / 2));
					}
					if (i == 2) {
						offset.pos.set(-100, (interior_size.y / 2 + sprite.get_size(metas).x / 2) * -1);
						offset.rotation = -90;
					}
					if (i == 3) {
						offset.pos.set(-100, (interior_size.y / 2 + sprite.get_size(metas).x / 2) *  1);
						offset.rotation = 90;
					}
					
					engine_physical.create_fixtures_component_from_renderable(
						step,
						[&](auto component){
							auto& group = component.get_fixture_group_data();

							group.filter = filters::see_through_dynamic_object();
							group.density = 1.0f;
							group.sensor = true;
							group.offsets_for_created_shapes[colliders_offset_type::SHAPE_OFFSET] = offset;

							engine_physical += component;
						}
					);

					engine_physical.get<components::fixtures>().set_owner_body(front);
					engine_physical.add_standard_components(step);

					this_engine_transform = engine_physical.get_logic_transform();
				}

				const vec2 engine_size = metas[assets::game_image_id::TRUCK_ENGINE].get_size();

				{
					particle_effect_input input;
					
					input.effect.id = assets::particle_effect_id::ENGINE_PARTICLES;
					input.effect.modifier.colorize = cyan;
					input.effect.modifier.scale_amounts = 6.7f;
					input.effect.modifier.scale_lifetimes = 0.45f;
					input.delete_entity_after_effect_lifetime = false;

					auto place_of_birth = this_engine_transform;

					if (i == 0 || i == 1) {
						place_of_birth.rotation += 180;
					}

					const auto engine_particles = input.create_particle_effect_entity(
						step,
						place_of_birth,
						front
					);

					auto& existence = engine_particles.get<components::particles_existence>();
					existence.distribute_within_segment_of_length = engine_size.y;

					engine_particles.add_standard_components(step);

					if (i == 0) {
						front.get<components::car>().acceleration_engine[i].physical = engine_physical;
						front.get<components::car>().acceleration_engine[i].particles = engine_particles;
					}
					if (i == 1) {
						front.get<components::car>().acceleration_engine[i].physical = engine_physical;
						front.get<components::car>().acceleration_engine[i].particles = engine_particles;
					}
					if (i == 2) {
						front.get<components::car>().left_engine.physical = engine_physical;
						front.get<components::car>().left_engine.particles = engine_particles;
					}
					if (i == 3) {
						front.get<components::car>().right_engine.physical = engine_physical;
						front.get<components::car>().right_engine.particles = engine_particles;
					}

					components::particles_existence::deactivate(engine_particles);
				}
			}

			{
				sound_effect_input in;
				in.effect.id = assets::sound_buffer_id::ENGINE;
				in.effect.modifier.repetitions = -1;
				in.delete_entity_after_effect_lifetime = false;

				const auto engine_sound = in.create_sound_effect_entity(step, spawn_transform, front);
				engine_sound.add_standard_components(step);

				front.get<components::car>().engine_sound = engine_sound;
				components::sound_existence::deactivate(engine_sound);
			}
		}

		front.add_standard_components(step);
		left_wheel.add_standard_components(step);
		interior.add_standard_components(step);

		return front;
	}

}