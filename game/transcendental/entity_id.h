#include "augs/misc/pool_id.h"
#include "game/transcendental/types_specification/all_components_declaration.h"

namespace augs {
	template<class...>
	class component_aggregate;
}

typedef augs::pool_id<typename put_all_components_into<augs::component_aggregate>::type> entity_id;
typedef augs::unversioned_id<typename put_all_components_into<augs::component_aggregate>::type> unversioned_entity_id;