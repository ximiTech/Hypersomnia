#pragma once
#include "augs/misc/delta.h"
#include "augs/misc/enum_boolset.h"

#include "augs/gui/gui_traversal_structs.h"
#include "augs/gui/gui_flags.h"

#include "augs/window_framework/event.h"

namespace augs {
	namespace gui {
		struct rect_node_data {
			augs::enum_boolset<flag> flags;

			ltrb rc; /* actual rectangle */

			rect_node_data(const xywh& rc = xywh());

			void set_default_flags();

			void set_flag(const flag f, const bool value = true);
			void unset_flag(const flag f);
			
			bool get_flag(const flag f) const;

			void set_scroll(const vec2);
			vec2 get_scroll() const;
		};

		template <class gui_element_variant_id>
		struct rect_node : rect_node_data {
			using gui_entropy = augs::gui::gui_entropy<gui_element_variant_id>;

			using rect_node_data::rect_node_data;

			template <class C, class gui_element_id>
			static void build_tree_data(
				const C context, 
				const gui_element_id this_id,
				const gui_element_variant_id parent_id
			) {
				const auto rc = this_id->rc;

				auto absolute_clipped_rect = rc;
				auto absolute_pos = vec2i(rc.get_position());
				auto absolute_clipping_rect = ltrb(0.f, 0.f, std::numeric_limits<int>::max() / 2.f, std::numeric_limits<int>::max() / 2.f);

				/* if we have parent */
				if (context.alive(parent_id)) {
					context(parent_id, [&](const auto& parent_rect) {
						auto& p = context.get_tree_entry(parent_id);
						const auto scroll = parent_rect->get_scroll();

						/* we have to save our global coordinates in absolute_xy */
						absolute_pos = p.get_absolute_pos() + rc.get_position() - scroll;
						absolute_clipped_rect = xywh(static_cast<float>(absolute_pos.x), static_cast<float>(absolute_pos.y), rc.w(), rc.h());

						/* and we have to clip by first clipping parent's rc_clipped */
						//auto* clipping = get_clipping_parent(); 
						//if(clipping) 
						absolute_clipped_rect.clip_by(p.get_absolute_clipping_rect());

						absolute_clipping_rect = p.get_absolute_clipping_rect();
					});
				}

				if (this_id->get_flag(flag::CLIP)) {
					absolute_clipping_rect.clip_by(absolute_clipped_rect);
				}

				/* align scroll only to be positive and not to exceed content size */
				//if (get_flag(flag::SNAP_SCROLL_TO_CONTENT_SIZE)) {
				//	this_call([](const auto& r) {
				//		r->clamp_scroll_to_right_down_corner();
				//	});
				//}

				const auto it = context.tree.try_emplace(this_id, rc, parent_id);
				ensure(it.second && "GUI graph is not DAG...");

				auto& this_tree_entry = (*it.first).second;

				this_tree_entry.set_absolute_clipped_rect(absolute_clipped_rect);
				this_tree_entry.set_absolute_pos(absolute_pos);
				this_tree_entry.set_absolute_clipping_rect(absolute_clipping_rect);

				this_id->for_each_child(context, this_id, [&](const auto& child_id) {
					child_id->build_tree_data(context, child_id, this_id);
				});
			}

			template <class C, class gui_element_id>
			static void consume_raw_input_and_generate_gui_events(
				const C context, 
				const gui_element_id this_id, 
				gui::raw_input_traversal& inf, 
				gui_entropy& entropies
			) {
				using namespace augs::event;

				auto& gr = context.get_rect_world();
				
				const auto state = context.get_input_state();
				const auto mouse_pos = state.mouse.pos;
				const auto msg = inf.change.msg;

				auto gui_event_lambda = [&](const gui_event ev, const int scroll_amount = 0) {
					entropies.post_event(this_id, { ev, scroll_amount });
				};

				if (this_id->get_flag(flag::ENABLE_DRAWING)) {
					if (this_id->get_flag(flag::ENABLE_DRAWING_OF_CHILDREN)) {
						this_id->for_each_child(context, this_id, [&](const auto& child_id) {
							if (child_id->get_flag(flag::ENABLE_DRAWING)) {
								child_id->consume_raw_input_and_generate_gui_events(context, child_id, inf, entropies);
							}
						});
					}

					if (!this_id->get_flag(flag::DISABLE_HOVERING)) {
						const auto absolute_clipped_rect = context.get_tree_entry(this_id).get_absolute_clipped_rect();
						const bool hovering = absolute_clipped_rect.good() && absolute_clipped_rect.hover(mouse_pos);

						if (context.dead(gr.rect_hovered)) {
							if (hovering && (msg == message::mousemotion || (inf.change.uses_mouse() && inf.change.was_any_key_pressed()))) {
								gui_event_lambda(gui_event::hover);
								gr.rect_hovered = this_id;
								inf.was_hovered_rect_visited = true;
							}
						}
						else if (gr.rect_hovered == gui_element_variant_id(this_id)) {
							const bool still_hovering = hovering;

							if (still_hovering) {
								if (msg == message::lup) {
									gui_event_lambda(gui_event::lup);
								}
								if (msg == message::ldown) {
									gr.rect_held_by_lmb = this_id;
									gr.ldrag_relative_anchor = mouse_pos - this_id->rc.get_position();
									gr.last_ldown_position = mouse_pos;
									gui_event_lambda(gui_event::ldown);
								}
								if (msg == message::mdown) {
									gui_event_lambda(gui_event::mdown);
								}
								if (msg == message::mdoubleclick) {
									gui_event_lambda(gui_event::mdoubleclick);
								}
								if (msg == message::ldoubleclick) {
									gr.rect_held_by_lmb = this_id;
									gr.ldrag_relative_anchor = mouse_pos - this_id->rc.get_position();
									gr.last_ldown_position = mouse_pos;
									gui_event_lambda(gui_event::ldoubleclick);
								}
								if (msg == message::ltripleclick) {
									gr.rect_held_by_lmb = this_id;
									gr.ldrag_relative_anchor = mouse_pos - this_id->rc.get_position();
									gr.last_ldown_position = mouse_pos;
									gui_event_lambda(gui_event::ltripleclick);
								}
								if (msg == message::rdown) {
									gr.rect_held_by_rmb = this_id;
									gui_event_lambda(gui_event::rdown);
								}
								if (msg == message::rdoubleclick) {
									gr.rect_held_by_rmb = this_id;
									gui_event_lambda(gui_event::rdoubleclick);
								}

								if (msg == message::wheel) {
									gui_event_lambda(gui_event::wheel, inf.change.scroll.amount);
								}

								if (msg == message::mousemotion) {
									if (gr.rect_held_by_lmb == gui_element_variant_id(this_id) && state.get_mouse_key(0) && absolute_clipped_rect.hover(state.mouse.ldrag)) {
										gui_event_lambda(gui_event::lpressed);
									}

									if (gr.rect_held_by_rmb == gui_element_variant_id(this_id) && state.get_mouse_key(1) && absolute_clipped_rect.hover(state.mouse.rdrag)) {
										gui_event_lambda(gui_event::rpressed);
									}
								}
							}
							else {
								// ensure(msg == message::mousemotion);
								gui_event_lambda(gui_event::hoverlost);
								unhover(context, this_id, inf, entropies);
							}

							inf.was_hovered_rect_visited = true;
						}
					}

					if (gr.rect_held_by_lmb == gui_element_variant_id(this_id) && msg == message::mousemotion && mouse_pos != gr.last_ldown_position) {
						const bool has_only_started = !gr.held_rect_is_dragged;
						
						gr.held_rect_is_dragged = true;
						gr.current_drag_amount = mouse_pos - gr.last_ldown_position;

						if (has_only_started) {
							gui_event_lambda(gui_event::lstarteddrag);
						}
						else {
							gui_event_lambda(gui_event::ldrag);
						}
					}
				}
			}

			//template <class C, class gui_element_id>
			//static void consume_gui_event(const C context, const gui_element_id this_id, const event_info e) {
				// try_to_make_this_rect_focused(context, this_id, e);
				//	scroll_content_with_wheel(context, e);
				//	try_to_enable_middlescrolling(context, e);
			//}

			template <class C, class gui_element_id>
			static void advance_elements(
				const C context, 
				const gui_element_id this_id, 
				const augs::delta dt
			) {
				this_id->for_each_child(context, this_id, [&](const auto& child_id) {
					child_id->advance_elements(
						context, 
						child_id, 
						dt
					);
				});
			}

			template <class C, class gui_element_id>
			static void respond_to_events(
				const C context, 
				const gui_element_id this_id, 
				const gui_entropy& entropies 
			) {
				this_id->for_each_child(context, this_id, [&](const auto& child_id) {
					child_id->respond_to_events(
						context, 
						child_id, 
						entropies
					);
				});
			}

			template <class C, class gui_element_id>
			static void rebuild_layouts(
				const C context, 
				const gui_element_id this_id
			) {
				this_id->for_each_child(context, this_id, [&](const auto& child_id) {
					child_id->rebuild_layouts(context, child_id);
				});
			}
			
			template <class C, class gui_element_id>
			static void draw(
				const C context, 
				const gui_element_id this_id
			) {
				if (!this_id->get_flag(flag::ENABLE_DRAWING)) {
					return;
				}

				draw_children(context, this_id);
			}

			template <class C, class gui_element_id>
			static void draw_children(
				const C context, 
				const gui_element_id this_id
			) {
				if (!this_id->get_flag(flag::ENABLE_DRAWING_OF_CHILDREN)) {
					return;
				}

				this_id->for_each_child(context, this_id, [&](const auto& child_id) {
					child_id->draw(context, child_id);
				});
			}

			template <class C, class gui_element_id, class L>
			static void for_each_child(
				const C, 
				const gui_element_id&, 
				L
			) {

			}

			template <class C, class gui_element_id>
			static void unhover(
				const C context, 
				const gui_element_id this_id, 
				gui::raw_input_traversal& inf, 
				gui_entropy& entropies
			) {
				auto& world = context.get_rect_world();

				auto gui_event_lambda = [&](const gui_event ev) {
					entropies.post_event(this_id, ev);
				};

				gui_event_lambda(gui_event::hoverlost);

				if (world.rect_held_by_lmb == gui_element_variant_id(this_id)) {
					gui_event_lambda(gui_event::loutdrag);
				}

				world.rect_hovered = gui_element_variant_id();
			}

			//template <class C, class gui_element_id>
			//void try_to_make_this_rect_focused(const C context, gui_element_id this_id, const gui::event_info e) {
			//	if (!get_flag(flag::FOCUSABLE)) return;
			//	
			//	auto& sys = context.get_rect_world();
			//
			//	if (e == gui_event::ldown ||
			//		e == gui_event::ldoubleclick ||
			//		e == gui_event::ltripleclick ||
			//		e == gui_event::rdoubleclick ||
			//		e == gui_event::rdown
			//		) {
			//		if (context.alive(sys.get_rect_in_focus())) {
			//			if (get_flag(flag::PRESERVE_FOCUS) || !context(sys.get_rect_in_focus(), [](const auto& r) { return r->get_flag(flag::PRESERVE_FOCUS); }))
			//				sys.set_focus(this_id);
			//		}
			//		else sys.set_focus(this_id);
			//	}
			//}

		};

			//bool is_scroll_clamped_to_right_down_corner() const;
			//void clamp_scroll_to_right_down_corner();

			//template <class C, class gui_element_id>
			//vec2 calculate_content_size(const C context) const {
			//	/* init on zero */
			//	ltrb content = ltrb(0.f, 0.f, 0.f, 0.f);
			//
			//	/* enlarge the content size by every child */
			//	context(this_id, [&content](const auto& r) {
			//		r->for_each_child(context, [&content](const auto& r) {
			//			if (r->get_flag(flag::ENABLE_DRAWING)) {
			//				content.contain_positive(r->rc);
			//			}
			//		});
			//	});
			//
			//	return content;
			//}

			//template <class C, class gui_element_id>
			//void scroll_content_with_wheel(const C context, const gui::event_info e) {
			//	auto& owner = e.owner;
			//	const auto& wnd = owner.state;
			//	const bool scrollable = get_flag(flag::SCROLLABLE);
			//
			//	const bool parent_alive = context.alive(parent);
			//	
			//	auto call_parent_wheel_callback = [this, &context, &owner]() {
			//		context(parent, [&owner, &context](auto& p) { p.consume_gui_event(context, gui::event_info(owner, gui_event::wheel)); });
			//	};
			//
			//	if (e == gui_event::wheel) {
			//		if (wnd.keys[augs::event::keys::key::SHIFT]) {
			//			int temp = static_cast<int>(scroll.x);
			//			if (scrollable) {
			//
			//				scroll.x -= wnd.mouse.scroll;
			//				clamp_scroll_to_right_down_corner();
			//			}
			//			if ((!scrollable || temp == scroll.x) && parent_alive) {
			//				call_parent_wheel_callback();
			//			}
			//		}
			//		else {
			//			int temp = static_cast<int>(scroll.x);
			//
			//			if (scrollable) {
			//				scroll.y -= wnd.mouse.scroll;
			//				clamp_scroll_to_right_down_corner();
			//			}
			//			if ((!scrollable || temp == scroll.y) && parent_alive) {
			//				call_parent_wheel_callback();
			//			}
			//		}
			//	}
			//}
			//
			//template <class C, class gui_element_id>
			//void try_to_enable_middlescrolling(const C context, const gui::event_info e) {
			//	auto& owner = e.owner;
			//	auto& wnd = owner.state;
			//
			//	if (e == gui_event::mdown || e == gui_event::mdoubleclick) {
			//		if (get_flag(flag::SCROLLABLE) && !content_size.inside(vec2(rc))) {
			//			owner.middlescroll.subject = this_id;
			//			owner.middlescroll.pos = wnd.mouse.pos;
			//			owner.set_focus(this_id);
			//		}
			//		else if (context.alive(parent)) {
			//			context(parent, [&context, e](auto& p) { p.consume_gui_event(context, e); })
			//		}
			//	}
			//}
			//
			//template <class C, class gui_element_id>
			//void scroll_to_view(const C context) {
			//	if (context.alive(parent)) {
			//		context(parent, [&](const auto& p) {
			//			const ltrb global = get_absolute_rect();
			//			const ltrb parent_global = p.get_absolute_rect();
			//			const vec2i off1 = vec2i(std::max(0.f, global.r + 2 - parent_global.r), std::max(0.f, global.b + 2 - parent_global.b));
			//			const vec2i off2 = vec2i(std::max(0.f, parent_global.l - global.l + 2 + off1.x), std::max(0.f, parent_global.t - global.t + 2 + off1.y));
			//			p.scroll += off1;
			//			p.scroll -= off2;
			//
			//			p.scroll_to_view(context);
			//		});
			//	}
			//}
	}
}