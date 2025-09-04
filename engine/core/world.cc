#include "config.h"
#include "world.h"


namespace Ecs {
    World::World() {
        auto eid = CreateEntity();
        AddComponent<TransformComponent, CT_TRANSFORM>(eid);
        AddComponent<CameraComponent, CT_CAMERA>(eid);

        eid = CreateEntity();
        AddComponent<ModelComponent, CT_MODEL>(eid);
        AddComponent<TransformComponent, CT_TRANSFORM>(eid);
    }

    World::~World() {}

    std::vector<EntityID> World::GetAllEntitiesBySignature(Signature sig) {
        std::vector<EntityID> res;

        for (auto e: this->m_entities) {
            auto esig = this->m_entitiesManager.GetSignature(e);
            auto comp = sig & esig;
            if (comp == sig) { res.push_back(e); }
        }

        return std::move(res);
    }

    void World::Start() {
        for (auto& [name, sys]: this->m_systemsManager) { sys->Start(this->GetAllEntitiesBySignature(sys->sig)); }
    }

    void World::Update(double dt) {
        for (auto& [name, sys]: this->m_systemsManager) { sys->Update(this->GetAllEntitiesBySignature(sys->sig), dt); }
    }

    void World::Draw() {
        for (auto& [name, sys]: this->m_systemsManager) { sys->Draw(this->GetAllEntitiesBySignature(sys->sig)); }
    }
} // namespace Ecs
