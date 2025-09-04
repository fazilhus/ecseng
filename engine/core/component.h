#pragma once

#include "ecs_types.h"
#include "pool.h"

#include <typeinfo>
#include <unordered_map>

#include "render/physics.h"
#include "render/renderdevice.h"


namespace Ecs {
    struct TransformComponent {};

    struct CameraComponent {};

    struct ModelComponent {
        Render::ModelId model_id;
    };

    struct PhysicsComponent {
        Physics::ColliderMeshId mesh_id;
        Physics::ColliderId collider_id;
    };

    template <typename ...Components>
    struct ComponentGroup {};

    using AllComponents = ComponentGroup<TransformComponent, CameraComponent, ModelComponent, PhysicsComponent>;

    class ComponentsManager {
    public:
        ComponentsManager();
        ~ComponentsManager();

        template <typename T>
        ComponentID GetComponentType() const {
            auto tname = typeid(T).name();
            assert(
                this->m_componentTypes.find(tname) != this->m_componentTypes.end() &&
                "component not registered registered"
            );

            return this->m_componentTypes.at(tname);
        }

        const std::unordered_map<const char*, BaseComponentPool*>& GetComponentPools() const { return m_components; }
        std::unordered_map<const char*, BaseComponentPool*>& GetComponentPools() { return m_components; }

        template <typename T>
        bool HasComponent(EntityID e) const { return this->GetComponentPool<T>().Has(e); }

        template <typename T>
        const T& GetComponent(EntityID e) const { return this->GetComponentPool<T>().Get(e); }

        template <typename T>
        T& GetComponent(EntityID e) { return this->GetComponentPool<T>().Get(e); }

        template <typename T, typename ...Args>
        void AddComponent(EntityID e, Args&& ...args) {
            this->GetComponentPool<T>()->Add(e, std::forward<Args>(args) ...);
        }

        template <typename T>
        void RemoveComponent(EntityID e) { this->GetComponentPool<T>().Remove(e); }

    private:
        std::unordered_map<const char*, ComponentID> m_componentTypes;
        std::unordered_map<const char*, BaseComponentPool*> m_components;
        ComponentID m_nextComponentType;

        template <typename T>
        const ComponentPool<T>* GetComponentPool() const {
            auto tname = typeid(T).name();
            assert(
                this->m_componentTypes.find(tname) != this->m_componentTypes.end() &&
                "component not registered registered"
            );
            return static_cast<ComponentPool<T>*>(this->m_components.at(tname));
        }

        template <typename T>
        ComponentPool<T>* GetComponentPool() {
            auto tname = typeid(T).name();
            assert(
                this->m_componentTypes.find(tname) != this->m_componentTypes.end() &&
                "component not registered registered"
            );
            return static_cast<ComponentPool<T>*>(this->m_components[tname]);
        }

        template <typename ...Component>
        void RegisterComponents(ComponentGroup<Component ...>) {
            ([&]() {
                auto tname = typeid(Component).name();
                assert(
                    this->m_componentTypes.find(tname) == this->m_componentTypes.end() && "component already registered"
                );

                this->m_componentTypes[tname] = this->m_nextComponentType;
                this->m_components[tname] = new ComponentPool<Component>{};
                this->m_nextComponentType <<= 1;
            }(), ...);
        }
    };
} // namespace Ecs
