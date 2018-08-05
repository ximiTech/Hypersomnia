#pragma once

constexpr unsigned SPACE_ATOMS_PER_UNIT = 1000;

enum class slot_function {
	INVALID,

	// GEN INTROSPECTOR enum class slot_function
	ITEM_DEPOSIT,

	GUN_CHAMBER,
	GUN_DETACHABLE_MAGAZINE,
	GUN_CHAMBER_MAGAZINE,
	GUN_RAIL,
	GUN_MUZZLE,

	PRIMARY_HAND,
	SECONDARY_HAND,

	BELT,
	SHOULDER,
	TORSO_ARMOR,
	HAT,
	// END GEN INTROSPECTOR
	COUNT
};