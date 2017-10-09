#pragma once
#include "augs/math/camera_cone.h"

#include "game/enums/game_intent_type.h"
#include "game/transcendental/entity_handle_declaration.h"
#include "game/detail/visible_entities.h"

#include "view/game_drawing_settings.h"
#include "view/game_gui/elements/hotbar_settings.h"
#include "view/viewables/all_viewables_declarations.h"
#include "view/necessary_resources.h"

class cosmos;
struct audiovisual_state;

namespace augs {
	struct renderer;
}

struct necessary_shaders;
struct necessary_fbos;

/* Require all */

using illuminated_rendering_fbos = necessary_fbos;
using illuminated_rendering_shaders = necessary_shaders;

struct illuminated_rendering_input {
	const const_entity_handle& viewed_character;
	const audiovisual_state& audiovisuals;
	const game_drawing_settings drawing;
	const necessary_images_in_atlas& necessary_images;
	const augs::baked_font& gui_font;
	const game_images_in_atlas_map& game_images;
	const vec2i screen_size;
	const double interpolation_ratio = 0.0;
	augs::renderer& renderer;
	const augs::graphics::texture& game_world_atlas;
	const illuminated_rendering_fbos& fbos;
	const illuminated_rendering_shaders& shaders;
	const camera_cone camera;
};

void illuminated_rendering(const illuminated_rendering_input);
