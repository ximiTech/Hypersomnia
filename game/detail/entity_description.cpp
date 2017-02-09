#include "entity_description.h"
#include "game/transcendental/entity_handle.h"
#include "game/detail/inventory_slot_handle.h"
#include "game/components/name_component.h"
#include "game/components/melee_component.h"
#include "game/components/gun_component.h"
#include "game/components/damage_component.h"
#include "game/components/container_component.h"
#include "game/components/sentience_component.h"
#include "game/components/item_component.h"
#include "game/detail/inventory_utils.h"
#include "game/detail/inventory_slot.h"
#include "game/detail/inventory_slot_id.h"
#include "game/transcendental/cosmos.h"
#include "augs/log.h"
#include <iomanip>

textual_description description_of_entity(const const_entity_handle id) {
	const auto& name = id.get<components::name>();
	
	auto result = description_by_entity_name(name.id);

	if (name.custom_nickname) {
		result.name = name.get_nickname();
	}

	return result;
}

std::wstring describe_properties(const const_entity_handle id) {
	std::wostringstream result;

	const auto& cosmos = id.get_cosmos();

	const auto* const melee = id.find<components::melee>();
	const auto* const gun = id.find<components::gun>();
	const auto* const damage = id.find<components::damage>();
	const auto* const container = id.find<components::container>();
	const auto* const item = id.find<components::item>();

	if (item) {
		if (item->categories_for_slot_compatibility.any()) {
			result << L"[color=vsblue]" << describe_item_compatibility_categories(item->categories_for_slot_compatibility) << L"[/color]\n";
		}
		
		const auto total_occupied = format_space_units(calculate_space_occupied_with_children(id));
		const auto per_charge = format_space_units(item->space_occupied_per_charge);

		result << "Occupies: [color=vscyan]" << total_occupied << " [/color]";
		
		if (item->charges > 1) {
			result << "[color=vsdarkgray](" << per_charge << L" each)[/color]";
		}
		else if (container && total_occupied != per_charge) {
			result << "[color=vsdarkgray](" << per_charge << L" if empty)[/color]";
		}

		result << L"\n";
	}

	if (gun) {
		if (cosmos[gun->magic_missile_definition].alive()) {
			result << typesafe_sprintf(L"Muzzle velocity: [color=vscyan]%x[/color]\nAmplification multiplier: [color=vscyan]%x[/color]\n", 
				(gun->muzzle_velocity.first + gun->muzzle_velocity.second) / 2, gun->damage_multiplier);
		}
		else {
			result << typesafe_sprintf(L"Muzzle velocity: [color=vscyan]%x[/color]\nDamage multiplier: [color=vscyan]%x[/color]\n",
				(gun->muzzle_velocity.first + gun->muzzle_velocity.second) / 2, gun->damage_multiplier);
		}
	}

	if (damage) {
		if (damage->amount > 0) {
			result << L"Base damage: [color=vscyan]" << damage->amount << L"[/color]\n";
		}
		else if (damage->amount < 0) {
			result << L"Restores health: [color=vscyan]" << -damage->amount << L"[/color]\n";
		}

		if (damage->constrain_distance) {
			result << L"Max distance: [color=vscyan]" << damage->max_distance << L"[/color]\n";
		}
		if (damage->constrain_lifetime) {
			result << L"Max lifetime: [color=vscyan]" << damage->max_lifetime_ms << L" ms[/color]\n";
		}
	}

	if (melee) {
		result << L"Swing duration: [color=vscyan]" << melee->swings[0].duration_ms << L" ms[/color]\n";
		result << L"Swing cooldown: [color=vscyan]" << melee->swings[0].cooldown_ms << L" ms[/color]\n";
	}

	const auto& depo = id[slot_function::ITEM_DEPOSIT];

	if (depo.alive()) {
		const auto children_space = format_space_units(depo.calculate_local_free_space());
		const auto with_parents_space = format_space_units(depo.calculate_real_free_space());

		result << L"Deposit space: [color=vsgreen]" << format_space_units(depo.calculate_real_free_space()) << L"[/color]/";

		if (children_space != with_parents_space)
			result << L"[color=vsyellow]" << format_space_units(depo.calculate_local_free_space()) << L"[/color]/";
			
		result << L"[color=vscyan]" << format_space_units(depo->space_available) << L"[/color]\n";
	}

	std::wstring out;

	const auto& bullet_round = id[sub_entity_name::BULLET_ROUND];

	if (bullet_round.alive()) {
		out = result.str() + describe_properties(bullet_round);
		return out;
	}
	else {
		out = result.str();
		return out.substr(0, out.length() - 1);
	}
}

std::wstring describe_slot(const const_inventory_slot_handle& id) {
	const auto text = describe_slot_function(id.get_id().type);
	const auto catcolor = id->for_categorized_items_only ? L"violet" : L"vsblue";

	return text.name + L"\n[color=vslightgray]Allows: [/color][color=" + catcolor + L"]" + describe_item_compatibility_categories(id->get_allowed_categories()) + L"[/color][color=vsdarkgray]\n" +
		text.details + L"[/color]";
}

std::wstring describe_entity(const const_entity_handle id) {
	const auto desc = description_of_entity(id);
	auto properties = describe_properties(id);
	
	if (!properties.empty()) {
		properties += L"\n";
	}

	return L"[color=white]" + desc.name + L"[/color]\n" + properties + L"[color=vsdarkgray]" + desc.details + L"[/color]";
}

std::wstring describe_sentience_meter(
	const const_entity_handle subject,
	const sentience_meter_type type
) {
	const auto& sentience = subject.get<components::sentience>();
	const auto value = sentience.call_on(type, [](const auto& m) { return m.get_value(); } );
	const auto maximum = sentience.call_on(type, [](const auto& m) { return m.get_maximum_value(); } );

	if (type == sentience_meter_type::HEALTH) {
		return typesafe_sprintf(L"[color=red]Health points:[/color] %x/%x\n[color=vsdarkgray]Stability of the physical body.[/color]", value, maximum);
	}

	if (type == sentience_meter_type::PERSONAL_ELECTRICITY) {
		return typesafe_sprintf(L"[color=cyan]Personal electricity:[/color] %x/%x\n[color=vsdarkgray]Mind-programmable matter.[/color]", value, maximum);
	}

	if (type == sentience_meter_type::CONSCIOUSNESS) {
		return typesafe_sprintf(L"[color=orange]Consciousness:[/color] %x/%x\n[color=vsdarkgray]Attunement of soul with the body.[/color]", value, maximum);
	}

	else return L"Unknown problem";
}

std::wstring describe_perk_meter(
	const const_entity_handle subject,
	const perk_meter_type type
) {
	const auto& sentience = subject.get<components::sentience>();

	if (type == perk_meter_type::HASTE) {
		return typesafe_sprintf(L"[color=green]Haste[/color]\n[color=vsdarkgray]You move faster.[/color]");
	}

	if (type == perk_meter_type::ELECTRIC_SHIELD) {
		return typesafe_sprintf(L"[color=green]Electric shield[/color]\n[color=vsdarkgray]Damage is absorbed by Personal Electricity\nrather than Health.[/color]");
	}

	else return L"Unknown problem";
}

std::wstring describe_spell(
	const const_entity_handle subject,
	const spell_type type
) {
	const auto& sentience = subject.get<components::sentience>();
	const auto spell_data = get_spell_data(type);

	const auto properties = typesafe_sprintf(L"Incantation: [color=yellow]%x[/color]\nPE to cast: [color=vscyan]%x[/color]\nCooldown: [color=vscyan]%x[/color]", 
		std::wstring(spell_data.incantation), spell_data.personal_electricity_required, spell_data.cooldown_ms);

	if (type == spell_type::HASTE) {
		return typesafe_sprintf(L"[color=green]Haste[/color]\n%x\n[color=vsdarkgray]Increases movement speed for 33 seconds.[/color]", properties);
	}

	if (type == spell_type::FURY_OF_THE_AEONS) {
		return typesafe_sprintf(L"[color=cyan]Fury of the Aeons[/color]\n%x\n[color=vsdarkgray]Causes instant damage around the caster.[/color]", properties);
	}

	if (type == spell_type::ULTIMATE_WRATH_OF_THE_AEONS) {
		return typesafe_sprintf(L"[color=cyan]Ultimate Wrath of the Aeons[/color]\n%x\n[color=vsdarkgray]Causes massive damage around the caster.\nRequires delay to initiate.[/color]", properties);
	}

	if (type == spell_type::ELECTRIC_TRIAD) {
		return typesafe_sprintf(L"[color=cyan]Electric Triad[/color]\n%x\n[color=vsdarkgray]Spawns three electric missiles\nhoming towards hostile entities.[/color]", properties);
	}

	else return L"Unknown problem";
}