#include "pixel_line_connector.h"
#include "augs/graphics/drawers.h"

std::vector<std::array<vec2i, 2>> get_connecting_pixel_lines(
	const rects::ltrb<float>& a,
	const rects::ltrb<float>& b
)
{
	using namespace augs::gui;

	vec2 ac = a.center();
	vec2 bc = b.center();

	vec2 aw2(a.w() / 2, 0);
	vec2 ah2(0, a.h() / 2);
	vec2 bw2(b.w() / 2, 0);
	vec2 bh2(0, b.h() / 2);

	if (ac.x >= b.r) {
		bool can_a_to_b = ac.y >= b.t && ac.y <= b.b;
		bool can_b_to_a = bc.y >= a.t && bc.y <= a.b;

		if (can_a_to_b && can_b_to_a) {
			if (a.h() > b.h())
				can_a_to_b = false;
			else
				can_b_to_a = false;
		}

		if (can_a_to_b) {
			return { { ac - aw2, vec2(b.r, ac.y) } };
		}
		else if (can_b_to_a) {
			return{ { bc + bw2, vec2(a.l, bc.y) } };
		}

		if (a.t >= bc.y) {
			return{ { bc + bw2, vec2(ac.x, bc.y) }, { vec2(ac.x, bc.y), vec2(ac.x, a.t) } };
		}
		else if (a.b <= bc.y) {
			return{ { bc + bw2, vec2(ac.x, bc.y) }, { vec2(ac.x, bc.y), vec2(ac.x, a.b) } };
		}
	}

	if (ac.x <= b.l) {
		bool can_a_to_b = ac.y >= b.t && ac.y <= b.b;
		bool can_b_to_a = bc.y >= a.t && bc.y <= a.b;

		if (can_a_to_b && can_b_to_a) {
			if (a.h() > b.h())
				can_a_to_b = false;
			else
				can_b_to_a = false;
		}

		if (can_a_to_b) {
			return{ { ac + aw2, vec2(b.l, ac.y) } };
		}
		else if (can_b_to_a) {
			return{ { bc - bw2, vec2(a.r, bc.y) } };
		}

		if (a.t >= bc.y) {
			return{ { bc - bw2, vec2(ac.x, bc.y) },{ vec2(ac.x, bc.y), vec2(ac.x, a.t) } };
		}
		else if (a.b <= bc.y) {
			return{ { bc - bw2, vec2(ac.x, bc.y) },{ vec2(ac.x, bc.y), vec2(ac.x, a.b) } };
		}
	}

	if (a.b < b.t) {
		bool can_a_to_b = ac.x >= b.l && ac.x <= b.r;
		bool can_b_to_a = bc.x >= a.l && bc.x <= a.r;

		if (can_a_to_b && can_b_to_a) {
			if (a.w() > b.w())
				can_a_to_b = false;
			else
				can_b_to_a = false;
		}

		if (can_a_to_b) {
			return{ { ac + ah2, vec2(ac.x, b.t) } };
		}
		else if (can_b_to_a) {
			return{ { bc - bh2, vec2(bc.x, a.b) } };
		}

		if (a.l >= bc.x) {
			return{ { bc - bh2, vec2(bc.x, ac.y) },{ vec2(bc.x, ac.y), vec2(a.l, ac.y) } };
		}
		else if (a.r <= bc.x) {
			return{ { bc - bh2, vec2(bc.x, ac.y) },{ vec2(bc.x, ac.y), vec2(a.r, ac.y) } };
		}
	}

	if (a.t > b.b) {
		bool can_a_to_b = ac.x >= b.l && ac.x <= b.r;
		bool can_b_to_a = bc.x >= a.l && bc.x <= a.r;

		if (can_a_to_b && can_b_to_a) {
			if (a.w() > b.w())
				can_a_to_b = false;
			else
				can_b_to_a = false;
		}

		if (can_a_to_b) {
			return{ { ac - ah2, vec2(ac.x, b.b) } };
		}
		else if (can_b_to_a) {
			return{ { bc + bh2, vec2(bc.x, a.t) } };
		}

		if (a.l >= bc.x) {
			return{ { bc + bh2, vec2(bc.x, ac.y) },{ vec2(bc.x, ac.y), vec2(a.l, ac.y) } };
		}
		else if (a.r <= bc.x) {
			return{ { bc + bh2, vec2(bc.x, ac.y) },{ vec2(bc.x, ac.y), vec2(a.r, ac.y) } };
		}
	}


	return { { } };
}


void draw_pixel_line_connector(
	const rects::ltrb<float>& a,
	const rects::ltrb<float>& b,
	const augs::gui::draw_info in,
	const augs::rgba col
) {
	for (const auto l : get_connecting_pixel_lines(a, b)) {
		augs::draw_line(in.v, l[0], l[1], 1, *assets::texture_id::BLANK, col);
	}
}