#include <thread>
#include <mutex>
#include <array>
#include "augs/global_libraries.h"
#include "application/game_window.h"

#include "game/assets/assets_manager.h"

#include "game/hardcoded_content/test_scenes/testbed.h"

#include "game/transcendental/types_specification/all_component_includes.h"
#include "game/view/viewing_session.h"
#include "game/transcendental/step_packaged_for_network.h"
#include "game/transcendental/cosmos.h"
#include "game/transcendental/logic_step.h"
#include "game/transcendental/cosmic_movie_director.h"
#include "game/transcendental/types_specification/all_messages_includes.h"
#include "game/transcendental/data_living_one_step.h"

#include "augs/audio/sound_buffer.h"
#include "augs/audio/sound_source.h"

#include "augs/filesystem/file.h"
#include "augs/misc/action_list.h"
#include "augs/misc/standard_actions.h"
#include "menu_setup.h"

#include "augs/gui/text/caret.h"

#include "application/menu_ui/menu_ui_root.h"
#include "application/menu_ui/menu_ui_context.h"

#include "application/menu_ui/appearing_text.h"
#include "application/menu_ui/creators_screen.h"

#include "augs/misc/http_requests.h"
#include "augs/templates/string_templates.h"

#include "augs/graphics/drawers.h"
#include "game/detail/visible_entities.h"
#include "application/config_lua_table.h"
#include "augs/misc/lua_readwrite.h"

#include "augs/audio/sound_samples_from_file.h"
#include "generated/introspectors.h"

using namespace augs::window::event::keys;
using namespace augs::gui::text;
using namespace augs::gui;

void menu_setup::process(
	game_window& window,
	viewing_session& session
) {
	const auto metas_of_assets = get_assets_manager().generate_logical_metas_of_assets();

	const vec2i screen_size = vec2i(window.get_screen_size());

	auto center = [&](auto& t) {
		t.target_pos = screen_size / 2 - get_text_bbox(t.get_total_target_text(), 0)*0.5f;
	};

	cosmos intro_scene(3000);

	augs::load_from_lua_table(session.config, "content/menu/config.lua");

	augs::single_sound_buffer menu_theme;
	augs::sound_source menu_theme_source;

	float gain_fade_multiplier = 0.f;

	if (session.config.music_volume > 0.f) {
		if (augs::file_exists(session.config.menu_theme_path)) {
			menu_theme.set_data(augs::get_sound_samples_from_file(session.config.menu_theme_path));

			menu_theme_source.bind_buffer(menu_theme);
			menu_theme_source.set_direct_channels(true);
			menu_theme_source.seek_to(static_cast<float>(session.config.start_menu_music_at_secs));
			menu_theme_source.set_gain(0.f);
			menu_theme_source.play();
		}
	}

	const style textes_style = style(assets::font_id::GUI_FONT, cyan);

	std::mutex news_mut;

	bool draw_cursor = false;
	bool roll_news = false;
	text_drawer latest_news_drawer;
	vec2 news_pos = vec2(static_cast<float>(screen_size.x), 5.f);

	std::thread latest_news_query([&latest_news_drawer, &session, &textes_style, &news_mut]() {
		auto result = augs::http_get_request(session.config.latest_news_url);
		const std::string delim = "newsbegin";

		const auto it = result.find(delim);

		if (it == std::string::npos) {
			return;
		}

		result = result.substr(it + delim.length());

		str_ops(result)
			.multi_replace_all({ "\r", "\n" }, "")
			;

		if (result.size() > 0) {
			const auto wresult = to_wstring(result);

			std::unique_lock<std::mutex> lck(news_mut);
			latest_news_drawer.set_text(format_as_bbcode(wresult, textes_style));
		}
	});

	augs::fixed_delta_timer timer = augs::fixed_delta_timer(5);
	timer.set_stepping_speed_multiplier(session.config.recording_replay_speed);

	// TODO: actually load a cosmos with its resources from a file/folder
	const bool is_intro_scene_available = session.config.menu_intro_scene_cosmos_path.size() > 0;

	if (is_intro_scene_available) {
		intro_scene.set_fixed_delta(session.config.default_tickrate);

		test_scenes::testbed().populate_world_with_entities(
			intro_scene,
			metas_of_assets,
			session.get_standard_post_solve()
		);
	}
	else {
		intro_scene.set_fixed_delta(session.config.default_tickrate);
	}

	const auto character_in_focus = is_intro_scene_available ?
		intro_scene.get_entity_by_name(L"player0")
		: intro_scene[entity_id()]
		;

	const auto title_size = metas_of_assets[assets::game_image_id::MENU_GAME_LOGO].get_size();

	ltrb title_rect;
	title_rect.set_position({ screen_size.x / 2.f - title_size.x / 2.f, 50.f });
	title_rect.set_size(title_size);

	rgba fade_overlay_color = { 0, 2, 2, 255 };
	rgba title_text_color = { 255, 255, 255, 0 };


	rgba tweened_menu_button_color = cyan;
	tweened_menu_button_color.a = 0;

	vec2i tweened_welcome_message_bg_size;

	menu_ui_rect_world menu_ui_rect_world;
	menu_ui_rect_world.last_state.screen_size = screen_size;

	menu_ui_root_in_context menu_ui_root_id;
	menu_ui_root menu_ui_root = screen_size;

	for (auto& m : menu_ui_root.menu_buttons) {
		m.hover_highlight_maximum_distance = 10.f;
		m.hover_highlight_duration_ms = 300.f;

		m.hover_sound.set_gain(session.config.sound_effects_volume);
		m.click_sound.set_gain(session.config.sound_effects_volume);
	}

	menu_ui_root.menu_buttons[menu_button_type::CONNECT_TO_OFFICIAL_UNIVERSE].set_appearing_caption(format(L"Login to official universe", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::BROWSE_UNOFFICIAL_UNIVERSES].set_appearing_caption(format(L"Browse unofficial universes", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::HOST_UNIVERSE].set_appearing_caption(format(L"Host universe", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::CONNECT_TO_UNIVERSE].set_appearing_caption(format(L"Connect to universe", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::LOCAL_UNIVERSE].set_appearing_caption(format(L"Local universe", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::EDITOR].set_appearing_caption(format(L"Editor", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::SETTINGS].set_appearing_caption(format(L"Settings", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::CREATORS].set_appearing_caption(format(L"Founders", textes_style));
	menu_ui_root.menu_buttons[menu_button_type::QUIT].set_appearing_caption(format(L"Quit", textes_style));

	menu_ui_root.set_menu_buttons_positions(screen_size);

	appearing_text credits1;
	credits1.target_text[0] = format(L"hypernet community presents", textes_style);
	center(credits1);

	appearing_text credits2;
	credits2.target_text = { format(L"A universe founded by\n", textes_style), format(L"Patryk B. Czachurski", textes_style) };
	center(credits2);

	std::vector<appearing_text*> intro_texts = { &credits1, &credits2 };

	appearing_text developer_welcome;
	{
		developer_welcome.population_interval = 60.f;

		developer_welcome.should_disappear = false;
		developer_welcome.target_text[0] = format(L"Thank you for building Hypersomnia.\n", textes_style);
		developer_welcome.target_text[1] = format(L"This message is not included in distributed executables.\n\
Our collective welcomes all of your suggestions and contributions.\n\
We wish you an exciting journey through architecture of our cosmos.\n", textes_style) +
		format(L"    ~hypernet community", style(assets::font_id::GUI_FONT, { 0, 180, 255, 255 }));

		developer_welcome.target_pos += screen_size - get_text_bbox(developer_welcome.get_total_target_text(), 0) - vec2(70.f, 70.f);
	}

	appearing_text hypersomnia_description;
	{
		hypersomnia_description.population_interval = 60.f;

		hypersomnia_description.should_disappear = false;
		hypersomnia_description.target_text[0] = format(L"- tendency of the omnipotent deity to immerse into inferior simulations,\nin spite of countless deaths experienced as a consequence.", { assets::font_id::GUI_FONT, {200, 200, 200, 255} });
		hypersomnia_description.target_pos = title_rect.left_bottom() + vec2(20, 20);
	}

	std::vector<appearing_text*> title_texts = { &developer_welcome, &hypersomnia_description };

	vec2i tweened_menu_button_size;
	vec2i target_tweened_menu_button_size = menu_ui_root.get_max_menu_button_size();

	creators_screen creators;
	creators.setup(textes_style, style(assets::font_id::GUI_FONT, { 0, 180, 255, 255 }), screen_size);

	augs::action_list intro_actions;

	{
		const bool play_credits = !session.config.skip_credits;

		if (play_credits) {
			intro_actions.push_blocking(act(new augs::delay_action(500.f)));
			intro_actions.push_non_blocking(act(new augs::tween_value_action<rgba_channel>(fade_overlay_color.a, 100, 6000.f)));
			intro_actions.push_non_blocking(act(new augs::tween_value_action<float>(gain_fade_multiplier, 1.f, 6000.f)));
			intro_actions.push_blocking(act(new augs::delay_action(2000.f)));
			
			for (auto& t : intro_texts) {
				t->push_actions(intro_actions);
			}
		}
		else {
			intro_actions.push_non_blocking(act(new augs::tween_value_action<float>(gain_fade_multiplier, 1.f, 6000.f)));
			fade_overlay_color.a = 100;
		}

		intro_actions.push_non_blocking(act(new augs::tween_value_action<rgba_channel>(fade_overlay_color.a, 20, 500.f)));

		intro_actions.push_blocking(act(new augs::tween_value_action<rgba_channel>(tweened_menu_button_color.a, 255, 250.f)));
		{
			augs::action_list welcome_tweens;

			const auto bbox = get_text_bbox(developer_welcome.get_total_target_text(), 0);
			
			welcome_tweens.push_blocking(act(new augs::tween_value_action<int>(tweened_welcome_message_bg_size.x, bbox.x, 500.f)));
			welcome_tweens.push_blocking(act(new augs::tween_value_action<int>(tweened_welcome_message_bg_size.y, bbox.y, 350.f)));
			
			intro_actions.push_non_blocking(act(new augs::list_action(std::move(welcome_tweens))));
		}

		intro_actions.push_blocking(act(new augs::tween_value_action<int>(tweened_menu_button_size.x, target_tweened_menu_button_size.x, 500.f)));
		intro_actions.push_blocking(act(new augs::tween_value_action<int>(tweened_menu_button_size.y, target_tweened_menu_button_size.y, 350.f)));

		intro_actions.push_non_blocking(act(new augs::tween_value_action<rgba_channel>(title_text_color.a, 255, 500.f)));
		
		intro_actions.push_non_blocking(act(new augs::set_value_action<bool>(roll_news, true)));
		intro_actions.push_non_blocking(act(new augs::set_value_action<vec2i>(menu_ui_rect_world.last_state.mouse.pos, window.get_screen_size()/2)));
		intro_actions.push_non_blocking(act(new augs::set_value_action<bool>(draw_cursor, true)));
		
		for (auto& t : title_texts) {
			augs::action_list acts;
			t->push_actions(acts);

#if !IS_PRODUCTION_BUILD
			intro_actions.push_non_blocking(act(new augs::list_action(std::move(acts))));
#endif
		}

		for (auto& m : menu_ui_root.menu_buttons) {
			augs::action_list acts;
			m.appearing_caption.push_actions(acts);

			intro_actions.push_non_blocking(act(new augs::list_action(std::move(acts))));
		}
	}

	cosmic_movie_director director;
	director.load_recording_from_file(session.config.menu_intro_scene_entropy_path);
	
	const bool is_recording_available = is_intro_scene_available && director.is_recording_available();
	
	timer.reset_timer();

	const auto initial_step_number = intro_scene.get_total_steps_passed();

	augs::action_list credits_actions;

	auto menu_callback = [&](const menu_button_type t){
		switch (t) {

		case menu_button_type::QUIT:
			should_quit = true;
			break;

		case menu_button_type::SETTINGS:
			session.show_settings = true;
			ImGui::SetWindowFocus("Settings");
			break;

		case menu_button_type::CREATORS:
			if (credits_actions.is_complete()) {
				credits_actions.push_blocking(act(new augs::tween_value_action<rgba_channel>(fade_overlay_color.a, 170, 500.f)));
				creators.push_into(credits_actions);
				credits_actions.push_blocking(act(new augs::tween_value_action<rgba_channel>(fade_overlay_color.a, 20, 500.f)));
			}
			break;

		default: break;
		}
	};

	if (is_recording_available) {
		while (intro_scene.get_total_time_passed_in_seconds() < session.config.rewind_intro_scene_by_secs) {
			const auto entropy = cosmic_entropy(director.get_entropy_for_step(intro_scene.get_total_steps_passed() - initial_step_number), intro_scene);

			intro_scene.advance_deterministic_schemata(
				{ entropy, metas_of_assets },
				[](auto) {},
				session.get_standard_post_solve()
			);
		}
	}

	while (!should_quit) {
		augs::machine_entropy new_machine_entropy;

		session.local_entropy_profiler.new_measurement();
		new_machine_entropy.local = window.collect_entropy(!session.config.debug_disable_cursor_clipping);
		session.local_entropy_profiler.end_measurement();
		
		if (draw_cursor) {
			session.perform_imgui_pass(
				new_machine_entropy.local,
				session.imgui_timer.extract<std::chrono::milliseconds>()
			);
		}

		process_exit_key(new_machine_entropy.local);

		{
			auto translated = session.config.controls.translate(new_machine_entropy.local);
			session.control_open_developer_console(translated.intents);
		}

		auto steps = timer.count_logic_steps_to_perform(intro_scene.get_fixed_delta());

		if (is_intro_scene_available) {
			while (steps--) {
				augs::renderer::get_current().clear_logic_lines();

				const auto entropy = is_recording_available ? 
					cosmic_entropy(director.get_entropy_for_step(intro_scene.get_total_steps_passed() - initial_step_number), intro_scene)
					: cosmic_entropy()
				;
				
				intro_scene.advance_deterministic_schemata(
					{ entropy, metas_of_assets },
					[](auto){},
					session.get_standard_post_solve()
				);
			}
		}

		session.set_master_gain(session.config.sound_effects_volume * 0.3f * gain_fade_multiplier);
		menu_theme_source.set_gain(session.config.music_volume * gain_fade_multiplier);

		thread_local visible_entities all_visible;
		session.get_visible_entities(all_visible, intro_scene);

		const augs::delta vdt =
			timer.get_stepping_speed_multiplier()
			* session.frame_timer.extract<std::chrono::milliseconds>()
		;

		session.advance_audiovisual_systems(
			intro_scene, 
			character_in_focus, 
			all_visible,
			vdt
		);
		
		auto& renderer = augs::renderer::get_current();
		renderer.clear_current_fbo();

		const auto current_time_seconds = intro_scene.get_total_time_passed_in_seconds();

		session.view(
			renderer, 
			intro_scene,
			character_in_focus,
			all_visible,
			timer.fraction_of_step_until_next_step(intro_scene.get_fixed_delta()), 
			augs::gui::text::format(typesafe_sprintf(L"Current time: %x", current_time_seconds), textes_style)
		);
		
		session.draw_color_overlay(renderer, fade_overlay_color);

		augs::draw_rect(
			renderer.get_triangle_buffer(),
			title_rect,
			assets::game_image_id::MENU_GAME_LOGO,
			title_text_color
		);

#if !IS_PRODUCTION_BUILD
		if (tweened_welcome_message_bg_size.non_zero()) {
			augs::draw_rect_with_border(renderer.get_triangle_buffer(),
				ltrb(developer_welcome.target_pos, tweened_welcome_message_bg_size).expand_from_center(vec2(6, 6)),
				{ 0, 0, 0, 180 },
				slightly_visible_white
			);
		}
#endif

		if (tweened_menu_button_size.non_zero()) {
			ltrb buttons_bg;
			buttons_bg.set_position(menu_ui_root.menu_buttons.front().rc.left_top());
			buttons_bg.b = menu_ui_root.menu_buttons.back().rc.b;
			buttons_bg.w(tweened_menu_button_size.x);

			augs::draw_rect_with_border(renderer.get_triangle_buffer(),
				buttons_bg.expand_from_center(vec2(14, 10)),
				{ 0, 0, 0, 180 },
				slightly_visible_white
			);
		}

		for (auto& t : intro_texts) {
			t->draw(renderer.get_triangle_buffer());
		}

		for (auto& t : title_texts) {
			t->draw(renderer.get_triangle_buffer());
		}

		creators.draw(renderer);

		if (roll_news) {
			news_pos.x -= vdt.in_seconds() * 100.f;

			{
				std::unique_lock<std::mutex> lck(news_mut);

				if (news_pos.x < -latest_news_drawer.get_bbox().x) {
					news_pos.x = static_cast<float>(screen_size.x);
				}

				latest_news_drawer.pos = news_pos;
				latest_news_drawer.draw_stroke(renderer.get_triangle_buffer());
				latest_news_drawer.draw(renderer.get_triangle_buffer());
			}
		}

		renderer.call_triangles();
		renderer.clear_triangles();

			menu_ui_rect_world::gui_entropy gui_entropies;

			menu_ui_rect_tree menu_ui_tree;
			menu_ui_context menu_ui_context(menu_ui_rect_world, menu_ui_tree, menu_ui_root);

			menu_ui_rect_world.build_tree_data_into_context(menu_ui_context, menu_ui_root_id);

			if (draw_cursor) {
				for (const auto& ch : new_machine_entropy.local) {
					menu_ui_rect_world.consume_raw_input_and_generate_gui_events(menu_ui_context, menu_ui_root_id, ch, gui_entropies);
				}

				ImGui::GetIO().MousePos = vec2(menu_ui_rect_world.last_state.mouse.pos);

				menu_ui_rect_world.call_idle_mousemotion_updater(menu_ui_context, menu_ui_root_id, gui_entropies);
			}

			menu_ui_rect_world.respond_to_events(menu_ui_context, menu_ui_root_id, gui_entropies);
			menu_ui_rect_world.advance_elements(menu_ui_context, menu_ui_root_id, vdt);

			menu_ui_root.set_menu_buttons_sizes(tweened_menu_button_size);
			menu_ui_root.set_menu_buttons_colors(tweened_menu_button_color);
			menu_ui_rect_world.rebuild_layouts(menu_ui_context, menu_ui_root_id);

			menu_ui_rect_world.draw(renderer.get_triangle_buffer(), menu_ui_context, menu_ui_root_id);

			for (size_t i = 0; i < menu_ui_root.menu_buttons.size(); ++i) {
				if (menu_ui_root.menu_buttons[i].click_callback_required) {
					menu_callback(static_cast<menu_button_type>(i));
					menu_ui_root.menu_buttons[i].click_callback_required = false;
				}
			}

			renderer.call_triangles();
			renderer.clear_triangles();

		renderer.draw_imgui(get_assets_manager());
		
		if (draw_cursor) {
			const auto mouse_pos = menu_ui_rect_world.last_state.mouse.pos;
			const auto gui_cursor_color = white;
		
			auto gui_cursor = assets::game_image_id::GUI_CURSOR;
		
			if (
				menu_ui_context.alive(menu_ui_rect_world.rect_hovered)
				|| ImGui::IsAnyItemHoveredWithHandCursor()
			) {
				gui_cursor = assets::game_image_id::GUI_CURSOR_HOVER;
			}

			if (ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeNWSE) {
				gui_cursor = assets::game_image_id::GUI_CURSOR_RESIZE_NWSE;
			}

			if (ImGui::GetMouseCursor() == ImGuiMouseCursor_TextInput) {
				gui_cursor = assets::game_image_id::GUI_CURSOR_TEXT_INPUT;
			}
			
			augs::draw_cursor(renderer.get_triangle_buffer(), mouse_pos, gui_cursor, gui_cursor_color);
			renderer.call_triangles();
			renderer.clear_triangles();
		}

		intro_actions.update(vdt);
		credits_actions.update(vdt);

		window.swap_buffers();
	}

	latest_news_query.detach();
}