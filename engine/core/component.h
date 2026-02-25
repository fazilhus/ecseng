#pragma once

#include "ecs_types.h"
#include "pool.h"

#include <typeindex>
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
            assert(
                this->m_componentTypes.contains(std::type_index(std::type_index(typeid(T)))) &&
                "component not registered registered"
            );

            return this->m_componentTypes.at(std::type_index(typeid(T)));
        }

        const std::unordered_map<std::type_index, BaseComponentPool*>& GetComponentPools() const { return m_components; }
        std::unordered_map<std::type_index, BaseComponentPool*>& GetComponentPools() { return m_components; }

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
        std::unordered_map<std::type_index, ComponentID> m_componentTypes;
        std::unordered_map<std::type_index, BaseComponentPool*> m_components;
        ComponentID m_nextComponentType;

        template <typename T>
        const ComponentPool<T>* GetComponentPool() const {
            assert(
                this->m_componentTypes.contains(std::type_index(typeid(T))) &&
                "component not registered registered"
            );
            return static_cast<ComponentPool<T>*>(this->m_components.at(std::type_index(typeid(T))));
        }

        template <typename T>
        ComponentPool<T>* GetComponentPool() {
            assert(
                this->m_componentTypes.contains(std::type_index(typeid(T))) &&
                "component not registered registered"
            );
            return static_cast<ComponentPool<T>*>(this->m_components[std::type_index(typeid(T))]);
        }

        template <typename ...Component>
        void RegisterComponents(ComponentGroup<Component ...>) {
            ([&]() {
                assert(
                    !this->m_componentTypes.contains(typeid(Component)) && "component already registered"
                );

                this->m_componentTypes[typeid(Component)] = this->m_nextComponentType;
                this->m_components[typeid(Component)] = new ComponentPool<Component>{};
                this->m_nextComponentType <<= 1;
            }(), ...);
        }
    };
} // namespace Ecs
