#include "config.h"
#include "world.h"

#include <ranges>

#include "system.h"


namespace Ecs {
    World::World() {
        m_entitiesManager.init();
        m_componentsManager.init();
        m_systemsManager = new SystemsManager();
        m_systemsManager->init(this);
        // auto eid = CreateEntity();
        // AddComponent<TransformComponent, CT_TRANSFORM>(eid);
        // AddComponent<CameraComponent, CT_CAMERA>(eid);
        //
        // eid = CreateEntity();
        // AddComponent<ModelComponent, CT_MODEL>(eid);
        // AddComponent<TransformComponent, CT_TRANSFORM>(eid);
    }

    World::~World() {
        m_systemsManager->deinit();
        delete m_systemsManager;
        m_componentsManager.deinit();
        m_entitiesManager.deinit();
    }

    EntityID World::CreateEntity()  {
        auto res = m_entitiesManager.CreateEntity();
        m_entities.insert(res);
        return res;
    }

    void World::DestroyEntity(const EntityID id) {
        m_to_be_deleted.push(id);
    }

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
        for (auto& [name, sys]: *this->m_systemsManager) {
            sys->Start(this->GetAllEntitiesBySignature(sys->sig));
        }
    }

    void World::BeforeFrame() {
        while (!m_to_be_deleted.empty()) {
            auto e = m_to_be_deleted.front();

            for (const auto& p: m_componentsManager.GetComponentPools() | std::views::values) {
                p->Remove(e);
            }

            m_entities.erase(e);
            m_entitiesManager.DestroyEntity(e);
        }
    }

    void World::PhysicsUpdate(float dt) {
        for (auto& [name, sys]: *this->m_systemsManager) {
            sys->PhysicsUpdate(this->GetAllEntitiesBySignature(sys->sig), dt);
        }
    }

    void World::Update(float dt) {
        for (auto& [name, sys]: *this->m_systemsManager) {
            sys->Update(this->GetAllEntitiesBySignature(sys->sig), dt);
        }
    }

    void World::BeforeDraw() {
        for (auto& [name, sys]: *this->m_systemsManager) {
            sys->BeforeDraw(this->GetAllEntitiesBySignature(sys->sig));
        }
    }

    void World::Draw() {
        for (auto& [name, sys]: *this->m_systemsManager) {
            sys->Draw(this->GetAllEntitiesBySignature(sys->sig));
        }
    }
} // namespace Ecs
