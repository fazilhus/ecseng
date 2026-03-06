#pragma once

#include "ecs_types.h"

#include <queue>


namespace Ecs {
    class EntityManager {
    public:
        EntityManager() = default;
        ~EntityManager() = default;

        void init();
        void deinit();

        EntityID CreateEntity();
        void DestroyEntity(EntityID id);

        Signature GetSignature(EntityID id) const;
        void SetSignature(EntityID id, Signature sig);

    private:
        std::priority_queue<EntityID, std::vector<EntityID>, std::greater<>> m_freeEntities;
        std::vector<EntityID> m_entities;
        std::vector<Signature> m_signatures;
    };
} // namespace Ecs
