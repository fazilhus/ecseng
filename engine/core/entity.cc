#include "config.h"
#include "entity.h"


namespace Ecs {

    void EntityManager::init() {
        for (EntityID i = 0; i < MaxEntityCount; ++i) { m_freeEntities.push(i); }

        m_signatures.resize(MaxEntityCount);
    }

    void EntityManager::deinit() {}

    EntityID EntityManager::CreateEntity() {
        if (this->m_freeEntities.empty()) { throw std::bad_alloc{}; }

        auto res = this->m_freeEntities.top();
        m_entities.push_back(res);
        this->m_freeEntities.pop();
        return res;
    }

    void EntityManager::DestroyEntity(EntityID id) {
        assert(id < MaxEntityCount);

        this->m_freeEntities.push(id);
        m_signatures[id] = 0;
    }

    Signature EntityManager::GetSignature(EntityID id) const {
        assert(id < MaxEntityCount);
        return m_signatures[id];
    }

    void EntityManager::SetSignature(EntityID id, Signature sig) {
        assert(id < MaxEntityCount);
        m_signatures[id] = sig;
    }
} // namespace Ecs
