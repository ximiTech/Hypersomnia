#pragma once
#include "augs/misc/streams.h"
#include "augs/window_framework/event.h"
#include <map>
#include <vector>
#include "game/transcendental/entity_id.h"
#include "game/messages/intent_message.h"

#include "game/detail/inventory/item_slot_transfer_request.h"
#include "augs/templates/container_templates.h"

#include "augs/misc/machine_entropy.h"

#include "game/assets/spell_id.h"
#include "augs/misc/container_with_small_size.h"

class cosmos;

template <class key>
struct basic_cosmic_entropy {
	typedef key key_type;

	// GEN INTROSPECTOR struct basic_cosmic_entropy class key
	augs::container_with_small_size<std::unordered_map<key, assets::spell_id>, unsigned char> cast_spells;
	augs::container_with_small_size<std::unordered_map<key, key_and_mouse_intent_vector>, unsigned char> intents_per_entity;
	augs::container_with_small_size<std::vector<basic_item_slot_transfer_request<key>>, unsigned short> transfer_requests;
	// END GEN INTROSPECTOR

	void override_transfers_leaving_other_entities(
		const cosmos&,
		std::vector<basic_item_slot_transfer_request<key>> new_transfers
	);

	size_t length() const;

	basic_cosmic_entropy& operator+=(const basic_cosmic_entropy& b);

	void clear();
};

struct cosmic_entropy;

struct guid_mapped_entropy : basic_cosmic_entropy<entity_guid> {
	typedef basic_cosmic_entropy<entity_guid> base;
	
	// GEN INTROSPECTOR struct guid_mapped_entropy
	// INTROSPECT BASE basic_cosmic_entropy<entity_guid>
	// END GEN INTROSPECTOR

	guid_mapped_entropy() = default;
	explicit guid_mapped_entropy(const cosmic_entropy&, const cosmos&);
	
	guid_mapped_entropy& operator+=(const guid_mapped_entropy& b) {
		base::operator+=(b);
		return *this;
	}

	bool operator!=(const guid_mapped_entropy&) const;
};

struct cosmic_entropy : basic_cosmic_entropy<entity_id> {
	typedef basic_cosmic_entropy<entity_id> base;

	// GEN INTROSPECTOR struct cosmic_entropy
	// INTROSPECT BASE basic_cosmic_entropy<entity_id>
	// END GEN INTROSPECTOR

	cosmic_entropy() = default;
	
	explicit cosmic_entropy(
		const guid_mapped_entropy&, 
		const cosmos&
	);
	
	explicit cosmic_entropy(
		const const_entity_handle controlled_entity, 
		const decltype(intents_per_entity[entity_id()])&
	);

	cosmic_entropy& operator+=(const cosmic_entropy& b) {
		base::operator+=(b);
		return *this;
	}
};