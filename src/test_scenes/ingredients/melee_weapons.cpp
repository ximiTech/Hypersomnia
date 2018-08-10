#include "test_scenes/ingredients/ingredients.h"
#include "game/cosmos/cosmos.h"
#include "game/cosmos/entity_handle.h"

#include "game/assets/ids/asset_ids.h"

#include "game/components/sender_component.h"
#include "game/components/missile_component.h"
#include "game/components/item_component.h"
#include "game/components/melee_component.h"

#include "game/detail/inventory/perform_transfer.h"

namespace test_flavours {
	void populate_melee_flavours(const populate_flavours_input in) {
		(void)in;
		{
#if TODO
			auto& meta = get_test_flavour(flavours, test_scene_flavour::URBAN_CYAN_MACHETE);

			invariants::render render_def;
			render_def.layer = render_layer::SMALL_DYNAMIC_BODY;

			meta.set(render_def);
			test_flavours::add_sprite(meta, caches, test_scene_image_id::URBAN_CYAN_MACHETE, white);

			test_flavours::add_lying_item_dynamic_body(meta);

			invariants::item item;
			item.space_occupied_per_charge = to_space_units("2.5");
			meta.set(item);

			invariants::missile missile;
			missile.destroy_upon_damage = false;
			missile.damage_upon_collision = false;
			missile.damage_amount = 50.f;
			missile.impulse_upon_hit = 1000.f;
			missile.constrain_lifetime = false;
			meta.set(missile);
#endif
		}
	}
}
