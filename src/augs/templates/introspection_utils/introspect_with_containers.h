#pragma once
#include "augs/templates/introspect.h"

#include "augs/templates/traits/is_variant.h"
#include "augs/templates/traits/is_optional.h"
#include "augs/templates/traits/container_traits.h"

namespace augs {
	template <class T>
	constexpr bool has_dynamic_content_v =
		is_optional_v<T>
		|| is_variant_v<T>
		|| is_container_v<T>
	;

	template <class F, class O>
	void on_dynamic_content(F callback, O& object) {
		using T = std::decay_t<O>;

		if constexpr(is_optional_v<T>) {
			if (object) {
				callback(*object);
			}
		}
		else if constexpr(is_variant_v<T>) {
			std::visit(
				[&](auto& resolved){ callback(resolved); }, 
				object
			);
		}
		else if constexpr(is_container_v<T>) {
			int i = 0;

			for (auto&& elem : object) {
				callback(elem, i++);
			}
		}
		else {
			static_assert(!has_dynamic_content_v<T>);
		}
	}
}
