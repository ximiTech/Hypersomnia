#include "melee_system.h"
#include "game/cosmos/cosmos.h"
#include "game/cosmos/entity_id.h"

#include "game/messages/intent_message.h"
#include "game/messages/melee_swing_response.h"

#include "game/components/melee_component.h"
#include "game/components/melee_fighter_component.h"
#include "game/components/missile_component.h"
#include "game/components/fixtures_component.h"
#include "game/components/torso_component.hpp"

#include "game/detail/frame_calculation.h"
#include "game/cosmos/entity_handle.h"
#include "game/cosmos/logic_step.h"
#include "game/cosmos/data_living_one_step.h"
#include "game/cosmos/for_each_entity.h"
#include "game/detail/entity_handle_mixins/inventory_mixin.hpp"
#include "game/detail/melee/like_melee.h"
#include "game/assets/animation_math.h"
#include "game/detail/inventory/perform_transfer.h"
#include "augs/math/convex_hull.h"
#include "game/enums/filters.h"
#include "game/detail/physics/physics_queries.h"
#include "augs/misc/enum/enum_bitset.h"
#include "game/messages/damage_message.h"

using namespace augs;

namespace components {
	bool melee_fighter::now_returning() const {
		return state == melee_fighter_state::RETURNING || state == melee_fighter_state::CLASH_RETURNING;
	}

	bool melee_fighter::fighter_orientation_frozen() const {
		const bool allow_rotation = state == melee_fighter_state::READY || state == melee_fighter_state::COOLDOWN;
		return !allow_rotation;
	}
}

void melee_system::advance_thrown_melee_logic(const logic_step step) {
	auto& cosm = step.get_cosmos();

	cosm.for_each_having<components::melee>([&](const auto& it) {
		auto& sender = it.template get<components::sender>();

		if (sender.is_set()) {
			if (!has_hurting_velocity(it)) {
				sender.unset();
				it.infer_rigid_body();
				it.infer_colliders();
			}
		}
	});
}

const auto reset_cooldown_v = real32(-1.f);

void melee_system::initiate_and_update_moves(const logic_step step) {
	auto& cosm = step.get_cosmos();
	const auto& dt = step.get_delta();
	const auto anims = cosm.get_logical_assets().plain_animations;

	std::vector<vec2> total_verts;
	const auto si = cosm.get_si();
	const auto& physics = cosm.get_solvable_inferred().physics;

	cosm.for_each_having<components::melee_fighter>([&](const auto& it) {
		const auto& fighter_def = it.template get<invariants::melee_fighter>();
		const auto& sentience_def = it.template get<invariants::sentience>();
		auto& sentience = it.template get<components::sentience>();

		auto& fighter = it.template get<components::melee_fighter>();

		auto& state = fighter.state;
		auto& anim_state = fighter.anim_state;
		auto& elapsed_ms = anim_state.frame_elapsed_ms;

		auto reset_fighter = [&]() {
			/* Avoid cheating by quick weapon switches */
			state = melee_fighter_state::COOLDOWN;

			auto& cooldown_ms = elapsed_ms;
			cooldown_ms = reset_cooldown_v;
		};

		const auto wielded_items = it.get_wielded_items();

		if (wielded_items.size() != 1) {
			reset_fighter();
			return;
		}

		const auto target_weapon = cosm[wielded_items[0]];

		if (!target_weapon.template has<components::melee>()) {
			reset_fighter();
			return;
		}

		const auto& torso = it.template get<invariants::torso>();
		const auto& stance = torso.calc_stance(it, wielded_items);

		const auto chosen_action = [&]() {
			for (std::size_t i = 0; i < hand_count_v; ++i) {
				const auto& hand_flag = sentience.hand_flags[i];

				if (hand_flag) {
					const auto action = it.calc_hand_action(i);
					return action.type;
				}
			}

			return weapon_action_type::COUNT;
		}();

		const auto dt_ms = dt.in_milliseconds();

		target_weapon.template dispatch_on_having_all<components::melee>(
			[&](const auto& typed_weapon) {
				const auto& melee_def = typed_weapon.template get<invariants::melee>();

				if (state == melee_fighter_state::READY) {
					if (chosen_action == weapon_action_type::COUNT) {
						return;
					}

					const auto& current_attack_def = melee_def.actions.at(chosen_action);

					if (augs::is_positive_epsilon(current_attack_def.cp_required)) {
						auto& consciousness = sentience.template get<consciousness_meter_instance>();
						const auto minimum_cp_to_attack = consciousness.get_maximum_value() * sentience_def.minimum_cp_to_sprint;

						const auto consciousness_damage_by_attack = consciousness.calc_damage_result(
							current_attack_def.cp_required,
							minimum_cp_to_attack
						);

						if (consciousness_damage_by_attack.excessive > 0) {
							return;
						}

						consciousness.value -= consciousness_damage_by_attack.effective;
					}

					state = melee_fighter_state::IN_ACTION;
					fighter.action = chosen_action;
					fighter.hit_obstacles.clear();
					fighter.hit_others.clear();
					fighter.previous_frame_transform = typed_weapon.get_logic_transform();

					typed_weapon.infer_colliders_from_scratch();

					const auto& body = it.template get<components::rigid_body>();
					const auto vel_dir = vec2(body.get_velocity()).normalize();

					if (const auto crosshair = it.find_crosshair()) {
						fighter.overridden_crosshair_base_offset = crosshair->base_offset;

						const auto cross_dir = vec2(crosshair->base_offset).normalize();

						const auto impulse_mult = (vel_dir.dot(cross_dir) + 1) / 2;

						const auto& movement_def  = it.template get<invariants::movement>();
						const auto conceptual_max_speed = std::max(1.f, movement_def.max_speed_for_animation);
						const auto current_velocity = body.get_velocity();
						const auto current_speed = current_velocity.length();

						const auto speed_mult = current_speed / conceptual_max_speed;

						{
							const auto min_effect = chosen_action == weapon_action_type::PRIMARY ? 0.8f : 0.f;
							const auto max_effect = chosen_action == weapon_action_type::PRIMARY ? 1.3f : 1.1f;

							const auto total_mult = std::min(max_effect, std::max(min_effect, speed_mult * impulse_mult));

							auto effect = current_attack_def.wielder_init_particles;
							effect.modifier.scale_amounts = total_mult;
							effect.modifier.scale_lifetimes = total_mult;

							effect.start(
								step,
								particle_effect_start_input::at_entity(it)
							);
						}

						body.apply_impulse(impulse_mult * cross_dir * body.get_mass() * current_attack_def.wielder_impulse);

						auto& movement = it.template get<components::movement>();
						movement.linear_inertia_ms += current_attack_def.wielder_inert_for_ms;

						{
							const auto min_effect = 0.88f;
							const auto max_effect = 1.f;

							const auto total_mult = std::min(max_effect, std::max(min_effect, speed_mult * impulse_mult));

							auto effect = current_attack_def.init_sound;
							effect.modifier.pitch = total_mult;

							effect.start(
								step,
								sound_effect_start_input::at_entity(typed_weapon).set_listener(it)
							);
						}

						{
							const auto& effect = current_attack_def.init_particles;

							effect.start(
								step,
								particle_effect_start_input::at_entity(typed_weapon)
							);
						}
					}

				}

				if (state == melee_fighter_state::COOLDOWN) {
					auto& cooldown_left_ms = elapsed_ms;

					if (cooldown_left_ms == reset_cooldown_v) {
						for (const auto& a : melee_def.actions) {
							if (cooldown_left_ms < a.cooldown_ms) {
								cooldown_left_ms = a.cooldown_ms;
							}
						}

						return;
					}

					const auto speed_mult = fighter_def.cooldown_speed_mult;

					cooldown_left_ms -= dt_ms * speed_mult;

					if (cooldown_left_ms <= 0.f) {
						state = melee_fighter_state::READY;
						cooldown_left_ms = 0.f;
					}

					return;
				}

				const auto prev_index = anim_state.frame_num;

				auto detect_damage = [&](
					const transformr& from,
					const transformr& to
				) {
					if (fighter.now_returning()) {
						return;
					}

					const auto impact_velocity = (to.pos - from.pos) * dt.in_steps_per_second();

					const auto image_id = typed_weapon.get_image_id();
					const auto& offsets = cosm.get_logical_assets().get_offsets(image_id);

					if (const auto& shape = offsets.non_standard_shape; !shape.empty()) {
						total_verts.clear();

						for (auto h : shape.original_poly) {
							h.mult(from);
							total_verts.emplace_back(h);
						}

						for (auto h : shape.original_poly) {
							h.mult(to);
							total_verts.emplace_back(h);
						}

						const auto subject_id = entity_id(it.get_id());

						auto detect_on = [&](const auto& convex) {
							const auto shape = to_polygon_shape(convex, si);
							const auto filter = predefined_queries::melee_query();

							auto handle_intersection = [&](
								const b2Fixture& fix,
								const vec2,
								const vec2 point_b
							) {
								const auto& point_of_impact = point_b;

								const auto victim = cosm[get_entity_that_owns(fix)];
								const auto victim_id = victim.get_id();

								const bool is_self = 
									victim_id == subject_id
									|| victim.get_owning_transfer_capability() == subject_id
								;

								if (is_self) {
									return callback_result::CONTINUE;
								}

								const auto hit_filter = fix.GetFilterData();
								const auto bs = augs::enum_bitset<filter_category>(static_cast<unsigned long>(hit_filter.categoryBits));

								const bool is_solid_obstacle = 
									bs.test(filter_category::WALL)
									|| bs.test(filter_category::GLASS_OBSTACLE)
								;

								const bool is_sentient = victim.template has<components::sentience>();

								auto& already_hit = [&]() -> auto& {
									if (is_sentient || is_solid_obstacle) {
										return fighter.hit_obstacles;
									}

									return fighter.hit_others;
								}();

								if (already_hit.size() < already_hit.max_size()) {
									const bool is_yet_unaffected = !found_in(already_hit, victim_id);

									const auto& body = it.template get<components::rigid_body>();

									if (is_solid_obstacle && already_hit.size() > 0 && victim_id == already_hit[0]) {
										body.apply_impulse(fighter.first_separating_impulse);
										fighter.first_separating_impulse = {};
									}
									else if (is_yet_unaffected) {
										already_hit.emplace_back(victim_id);

										const auto& current_attack_def = melee_def.actions.at(fighter.action);
										const auto& def = current_attack_def.damage;

										if (is_solid_obstacle && already_hit.size() == 1) {
											{
												auto& current = sentience.rotation_inertia_ms;
												const auto& bonus = current_attack_def.obstacle_hit_rotation_inertia_ms;

												current = std::max(current, bonus);
											}

											if (const auto crosshair = it.find_crosshair()) {
												const auto& kickback = current_attack_def.obstacle_hit_kickback_impulse;

												const auto point_dir = (from.pos - point_of_impact).normalize();

												const auto ray_a = from.pos;
												const auto ray_b = point_of_impact - point_dir * 50;

												const auto ray = physics.ray_cast_px(
													cosm.get_si(),
													ray_a,
													ray_b,
													predefined_queries::melee_solid_obstacle_query(),
													typed_weapon
												);

												if (ray.hit) {
													const auto& n = ray.normal;

													if (DEBUG_DRAWING.draw_melee_info) {
														DEBUG_PERSISTENT_LINES.emplace_back(
															cyan,
															ray_a,
															ray_b
														);

														DEBUG_PERSISTENT_LINES.emplace_back(
															yellow,
															ray.intersection,
															ray.intersection + n * 100
														);
													}

													{
														const auto inter_dir = (ray_a - ray.intersection).normalize();

														impulse_input in;
														in.angular = n.cross(inter_dir) * current_attack_def.obstacle_hit_recoil_mult;
														it.apply_crosshair_recoil(in);

														const auto parallel_mult = n.dot(inter_dir);
														const auto considered_mult = std::max(parallel_mult - 0.4f, 0.f);
														const auto total_impulse_mult = considered_mult * kickback * body.get_mass() / 3;
														const auto total_impulse = total_impulse_mult * n;

														const auto vel = body.get_velocity();
														const auto target_vel = vec2(vel).reflect(n);

														const auto velocity_conservation = 0.6f;
														body.set_velocity(target_vel * velocity_conservation);

														body.apply_impulse(total_impulse);
														fighter.first_separating_impulse = total_impulse;
													}
												}
											}

											auto& movement = it.template get<components::movement>();
											movement.linear_inertia_ms += current_attack_def.obstacle_hit_linear_inertia_ms;
										}

										messages::damage_message damage_msg;

										const auto speed = it.get_effective_velocity().length();
										const auto bonus_mult = speed * current_attack_def.bonus_damage_speed_ratio;
										const auto mult = 1.f + bonus_mult;

										damage_msg.damage = def;
										damage_msg.damage *= mult;

										damage_msg.type = adverse_element_type::FORCE;
										damage_msg.origin = damage_origin(typed_weapon);
										damage_msg.subject = victim;
										damage_msg.impact_velocity = impact_velocity;
										damage_msg.point_of_impact = point_of_impact;

										step.post_message(damage_msg);
									}
								}

								return callback_result::CONTINUE;
							};

							physics.for_each_intersection_with_polygon(
								si,
								convex,
								filter,
								handle_intersection
							);
						};

						const auto max_v = b2_maxPolygonVertices;
						auto hull = augs::convex_hull(total_verts);

						/* Double-duty */
						auto& acceptable = total_verts;
						acceptable.clear();

						while (hull.size() > max_v) {
							acceptable.assign(hull.begin(), hull.begin() + max_v);
							hull.erase(hull.begin() + 1, hull.begin() + max_v - 1);

							detect_on(acceptable);
						}

						detect_on(hull);
					}
					else {
						ensure(false && "A knife is required to have a non-standard shape defined.");
					}
				};

				auto infer_if_different_frames = [&]() {
					const auto next_index = anim_state.frame_num;

					if (next_index != prev_index) {
						const auto damage_from = fighter.previous_frame_transform;
						typed_weapon.infer_colliders_from_scratch();
						const auto damage_to = typed_weapon.get_logic_transform();
						fighter.previous_frame_transform = damage_to;

						detect_damage(damage_from, damage_to);

						return true;
					}

					return false;
				};

				const auto& action = fighter.action;
				const auto& stance_anims = stance.actions[action];

				if (state == melee_fighter_state::IN_ACTION) {
					if (const auto* const current_anim = mapped_or_nullptr(anims, stance_anims.perform)) {
						const auto& f = current_anim->frames;

						if (!anim_state.advance(dt_ms, f)) {
							/* Animation is in progress. */
							infer_if_different_frames();
						}
						else {
							/* The animation has finished. */
							state = melee_fighter_state::RETURNING;
							anim_state.frame_num = 0;
						}
					}

					return;
				}

				if (fighter.now_returning()) {
					if (const auto* const current_anim = mapped_or_nullptr(anims, stance_anims.returner)) {
						const auto& f = current_anim->frames;

						if (!anim_state.advance(dt_ms, f)) {
							/* Animation is in progress. */
							infer_if_different_frames();
						}
						else {
							/* The animation has finished. */
							state = melee_fighter_state::COOLDOWN;
							anim_state.frame_num = 0;

							const auto total_ms = ::calc_total_duration(f);

							const auto& current_attack_def = melee_def.actions.at(action);

							if (action == weapon_action_type::PRIMARY) {
								const auto new_hand = it.get_first_free_hand();
								auto request = item_slot_transfer_request::standard(typed_weapon, new_hand);
								request.params.perform_recoils = false;

								perform_transfer_no_step(request, cosm);
							}
							else {
								typed_weapon.infer_colliders_from_scratch();
							}

							auto& cooldown_left_ms = elapsed_ms;
							cooldown_left_ms = std::max(0.f, current_attack_def.cooldown_ms - total_ms);
						}
					}

					return;
				}

			}
		);
	});
}