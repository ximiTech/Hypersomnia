#pragma once
#include "game/detail/gui/gui_element_location.h"

class gui_tree_entry {
	const augs::gui::rect_node_data& node_data;
	gui_element_location parent;
	vec2 absolute_position;
public:
	gui_tree_entry(const augs::gui::rect_node_data& node_data) : node_data(node_data) {}

	void set_parent(const gui_element_location& id) {
		parent = id;
	}

	void set_absolute_clipping_rect(const rects::ltrb<float>&) {

	}

	void set_absolute_clipped_rect(const rects::ltrb<float>&) {

	}

	void set_absolute_pos(const vec2& v) {
		absolute_position = v;
	}

	gui_element_location get_parent() const {
		return parent;
	}

	rects::ltrb<float> get_absolute_rect() const {
		return rects::xywh<float>(absolute_position.x, absolute_position.y, node_data.rc.w(), node_data.rc.h());
	}

	rects::ltrb<float> get_absolute_clipping_rect() const {
		return rects::ltrb<float>(0.f, 0.f, std::numeric_limits<int>::max() / 2.f, std::numeric_limits<int>::max() / 2.f);
	}

	rects::ltrb<float> get_absolute_clipped_rect() const {
		return node_data.rc;
	}

	vec2 get_absolute_pos() const {
		return absolute_position;
	}
};

typedef std::unordered_map<gui_element_location, gui_tree_entry> gui_element_tree;