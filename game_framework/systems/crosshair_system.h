#pragma once
#include "entity_system/processing_system.h"

#include "../components/crosshair_component.h"
#include "../components/transform_component.h"

using namespace augs;


class crosshair_system : public processing_system_templated<components::transform, components::crosshair> {
public:
	void consume_events(world&);
	void process_entities(world&);
};