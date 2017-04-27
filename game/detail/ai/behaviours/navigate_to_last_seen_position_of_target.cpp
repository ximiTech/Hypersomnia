#include "game/detail/ai/behaviours.h"
#include "game/transcendental/entity_id.h"

#include "game/components/attitude_component.h"
#include "game/components/movement_component.h"
#include "game/components/pathfinding_component.h"
#include "game/detail/entity_scripts.h"
#include "game/transcendental/cosmos.h"
#include "game/transcendental/logic_step.h"

namespace behaviours {
	tree::goal_availability navigate_to_last_seen_position_of_target::goal_resolution(tree::state_of_traversal& t) const {
		auto subject = t.subject;
		auto& cosmos = t.step.cosm;
		auto& attitude = subject.get<components::attitude>();
		auto currently_attacked_visible_entity = cosmos[attitude.currently_attacked_visible_entity];

		if (currently_attacked_visible_entity.dead() && attitude.is_alert && !attitude.last_seen_target_position_inspected) {
			return tree::goal_availability::SHOULD_EXECUTE;
		}

		return tree::goal_availability::ALREADY_ACHIEVED;
	}

	void navigate_to_last_seen_position_of_target::execute_leaf_goal_callback(tree::execution_occurence o, tree::state_of_traversal& t) const {
		auto subject = t.subject;
		auto& attitude = subject.get<components::attitude>();
		auto& movement = subject.get<components::movement>();
		auto& pathfinding = subject.get<components::pathfinding>();

		if (o == tree::execution_occurence::FIRST) {
			pathfinding.start_pathfinding(attitude.last_seen_target_position);
		}
		else if (o == tree::execution_occurence::LAST) {
			movement.reset_movement_flags();
			pathfinding.stop_and_clear_pathfinding();
		}
		else {
			if (pathfinding.has_pathfinding_finished()) {
				attitude.last_seen_target_position_inspected = true;
			}
			else {
				movement.set_flags_from_closest_direction(pathfinding.get_current_navigation_point() - subject.get_logic_transform().pos);
			}
		}
	}
}