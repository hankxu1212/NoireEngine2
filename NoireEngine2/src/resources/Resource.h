#pragma once

#include <typeindex>
#include "utils/NonCopyable.h"
#include "core/Core.h"

namespace Noire {
	/**
	 * @brief A managed resource object. Implementations contain Create functions that can take a node object or pass parameters to the constructor.
	 */
	class NE_API Resource : NonCopyable {
	public:
		Resource() = default;
		virtual ~Resource() = default;

		virtual std::type_index getTypeIndex() const = 0;
	};
}
