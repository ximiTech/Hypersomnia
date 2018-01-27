#pragma once
// disables the warning due to type name length exceeded
#pragma warning(disable : 4503)

#include "augs/templates/type_list.h"
#include "augs/templates/type_matching_and_indexing.h"

#include "game/components/transform_component_declaration.h"
#include "game/components/sprite_component_declaration.h"
#include "game/components/polygon_component_declaration.h"

namespace invariants {
	struct gun;
	struct render;
	struct shape_polygon;
	struct trace;
	struct interpolation;
	struct flags;
	struct fixtures;
	struct rigid_body;
	struct container;
	struct item;
	struct missile;
}

namespace components {
	struct animation;
	struct behaviour_tree;
	struct crosshair;
	struct missile;
	struct gun;
	struct movement;
	struct pathfinding;
	struct rigid_body;
	struct car;
	struct driver;
	struct specific_colliders_connection;
	struct item;
	struct force_joint;
	struct item_slot_transfers;
	struct type;
	struct trace;
	struct melee;
	struct sentience;
	struct attitude;
	struct processing;
	struct guid;
	struct child;
	struct interpolation;
	struct light;
	struct wandering_pixels;
	struct sound_existence;
	struct explosive;
	struct catridge;
	struct shape_polygon;
	struct shape_circle;
	struct motor_joint;
	struct hand_fuse;
	struct sender;

	struct all_inferred_state;
}

template <template <class...> class List>
using component_list_t = List<
	components::animation,
	components::behaviour_tree,
	components::crosshair,
	components::missile,
	components::gun,
	components::movement,
	components::pathfinding,
	components::rigid_body,
	components::specific_colliders_connection,
	components::transform,
	components::car,
	components::driver,
	components::item,
	components::force_joint,
	components::item_slot_transfers,
	components::type,
	components::trace,
	components::melee,
	components::sentience,
	components::attitude,
	components::processing,
	components::guid,
	components::child,
	components::interpolation,
	components::light,
	components::wandering_pixels,
	components::explosive,
	components::catridge,
	components::shape_circle,
	components::motor_joint,
	components::hand_fuse,
	components::sender,

	components::all_inferred_state
>;

template <template <class...> class List>
using invariant_list_t = List<
	invariants::gun,
	invariants::render,
	invariants::shape_polygon,
	invariants::polygon,
	invariants::sprite,
	invariants::trace,
	invariants::flags,
	invariants::interpolation,
	invariants::fixtures,
	invariants::rigid_body,
	invariants::container,
	invariants::item,
	invariants::missile
>;

template <class... Types>
struct type_count {
	static const unsigned value = sizeof...(Types);
};

class cosmos;

constexpr unsigned COMPONENTS_COUNT = component_list_t<type_count>::value;
constexpr unsigned INVARIANTS_COUNT = invariant_list_t<type_count>::value;

template <class D>
static constexpr auto invariant_index_v = index_in_list_v<D, invariant_list_t<type_list>>;

template <class D>
static constexpr bool is_invariant_v = is_one_of_list_v<D, invariant_list_t<type_list>>;