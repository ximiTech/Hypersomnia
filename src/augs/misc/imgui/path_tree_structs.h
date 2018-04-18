#pragma once
#include "augs/misc/imgui/browsed_path_entry_base.h"
#include "augs/misc/imgui/imgui_control_wrappers.h"

struct path_tree_settings {
	// GEN INTROSPECTOR struct path_tree_settings
	bool linear_view = true;
	bool prettify_filenames = true;
	// END GEN INTROSPECTOR

	std::string get_prettified(const std::string& filename) const;

	void do_tweakers();
	void do_name_location_columns() const;
};
