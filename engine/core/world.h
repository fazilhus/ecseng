#pragma once

#include "entity.h"
#include "component.h"
#include "pool.h"

#include <unordered_set>


namespace Ecs {
    class SystemsManager;
    class World {
    public:
        World();
        ~World();

        EntityID CreateEntity();
        void DestroyEntity(EntityID id);

        template <typename T>
        bool HasComponent(EntityID id) const { return m_componentsManager.HasComponent<T>(id); }

        template <typename T>
        const T& GetComponent(EntityID id) const { return m_componentsManager.GetComponent<T>(id); }

        template <typename T>
        T& GetComponent(EntityID id) { return m_componentsManager.GetComponent<T>(id); }

        template <typename T, ComponentTypes Type, typename ...Args>
        void AddComponent(EntityID id, Args&& ...args) {
            m_componentsManager.AddComponent<T>(id, std::forward<Args>(args) ...);
            auto e_sig = m_entitiesManager.GetSignature(id);
            m_entitiesManager.SetSignature(id, e_sig | Type);
        }

        template <typename T>
        bool RemoveComponent(EntityID id) { return m_componentsManager.RemoveComponent<T>(id); }

        std::vector<EntityID> GetAllEntitiesBySignature(Signature sig);
        template <ComponentID ...Comps>
        std::vector<EntityID> GetAllEntitiesByComponentIDs() {
            Signature sig{};
            for (auto s : {Comps...}) {
                sig |= s;
            }
            return GetAllEntitiesBySignature(sig);
        }

        void Start();
        void BeforeFrame();
        void PhysicsUpdate(float dt);
        void Update(float dt);
        void BeforeDraw();
        void Draw();

    private:
        std::unordered_set<EntityID> m_entities;
        std::queue<EntityID> m_to_be_deleted;
        EntityManager m_entitiesManager;
        ComponentsManager m_componentsManager;
        SystemsManager* m_systemsManager;
    };
} // namespace Ecs
