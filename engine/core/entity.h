#pragma once
#include <array>
#include <queue>

#include "idpool.h"

namespace Ecs {

	struct EntityId {
        uint32_t index : 22; // 4M concurrent meshes
        uint32_t generation : 10; // 1024 generations per index

        constexpr static EntityId Create(uint32_t id) {
            EntityId ret{ id & 0x003FFFFF, (id & 0xFFC00000) >> 22 };
            return ret;
        }

        explicit constexpr operator uint32_t() const {
            return ((generation << 22) & 0xFFC00000ul) + (index & 0x003FFFFFul);
        }

        static constexpr EntityId Invalid() {
            return Create(0xFFFFFFFF);
        }

        constexpr uint32_t HashCode() const {
            return index;
        }

        const bool operator==(const EntityId& rhs) const { return uint32_t(*this) == uint32_t(rhs); }
        const bool operator!=(const EntityId& rhs) const { return uint32_t(*this) != uint32_t(rhs); }
        const bool operator<(const EntityId& rhs) const { return index < rhs.index; }
        const bool operator>(const EntityId& rhs) const { return index > rhs.index; }
	};

	constexpr std::uint32_t MAX_ENTITIES = 256;

    static Util::IdPool<EntityId> entityPool;

} // namespace Ecs