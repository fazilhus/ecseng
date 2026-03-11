#pragma once

#include "ecs_types.h"
#include "pool.h"

#include <typeindex>
#include <unordered_map>

#include "render/physics.h"
#include "render/renderdevice.h"


namespace Ecs {

    struct TransformComponent {
        glm::mat4 transform = glm::mat4(1.0f);
        glm::vec3 pos{};
        glm::quat rot{};
        glm::vec3 scale{};

        TransformComponent(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale);
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(TransformComponent&&) = default;
        TransformComponent& operator=(const TransformComponent&) = default;
        TransformComponent& operator=(TransformComponent&&) = default;
    };

    struct CameraComponent {
        glm::mat4 view{};
        glm::mat4 projection{};
        glm::vec3 cam_pos{};
        glm::vec3 cam_offset{};
        float cam_smooth{};

        CameraComponent(const glm::mat4& v, const glm::mat4& p);
        CameraComponent(const CameraComponent&) = default;
        CameraComponent(CameraComponent&&) = default;
        CameraComponent& operator=(const CameraComponent&) = default;
        CameraComponent& operator=(CameraComponent&&) = default;
    };

    struct ModelComponent {
        Render::ModelId model_id;

        ModelComponent(const Render::ModelId model_id) : model_id(model_id) {}
        ModelComponent(const ModelComponent&) = default;
        ModelComponent(ModelComponent&&) = default;
        ModelComponent& operator=(const ModelComponent&) = default;
        ModelComponent& operator=(ModelComponent&&) = default;
    };

    struct ColliderComponent {
        Physics::ColliderId collider_id;

        ColliderComponent(const Physics::ColliderId cid) : collider_id(cid) {}
        ColliderComponent(const ColliderComponent&) = default;
        ColliderComponent(ColliderComponent&&) = default;
        ColliderComponent& operator=(const ColliderComponent&) = default;
        ColliderComponent& operator=(ColliderComponent&&) = default;
    };

    struct PlayerCharacterComponent {
        EntityID heading{};
        glm::vec3 linearVelocity = glm::vec3(0);

        float normalSpeed = 1.0f;
        float boostSpeed = normalSpeed * 2.0f;
        float accelerationFactor = 1.0f;

        float currentSpeed = 0.0f;

        float rotationZ = 0;
        float rotXSmooth = 0;
        float rotYSmooth = 0;
        float rotZSmooth = 0;

        PlayerCharacterComponent() {}
        PlayerCharacterComponent(const PlayerCharacterComponent&) = default;
        PlayerCharacterComponent(PlayerCharacterComponent&&) = default;
        PlayerCharacterComponent& operator=(const PlayerCharacterComponent&) = default;
        PlayerCharacterComponent& operator=(PlayerCharacterComponent&&) = default;
    };

    struct AICharacterComponent {
        EntityID heading{};
        glm::vec3 linearVelocity = glm::vec3(0);

        float normalSpeed = 1.0f;

        AICharacterComponent() {}
        AICharacterComponent(const AICharacterComponent&) = default;
        AICharacterComponent(AICharacterComponent&&) = default;
        AICharacterComponent& operator=(const AICharacterComponent&) = default;
        AICharacterComponent& operator=(AICharacterComponent&&) = default;
    };

    struct CollisionComponent {
        std::vector<glm::vec3> rays;

        CollisionComponent(const std::vector<glm::vec3>& v) : rays(v) {}
        CollisionComponent(const CollisionComponent&) = default;
        CollisionComponent(CollisionComponent&&) = default;
        CollisionComponent& operator=(const CollisionComponent&) = default;
        CollisionComponent& operator=(CollisionComponent&&) = default;
    };

    struct WaypointComponent {
        EntityID prev{}, next{};

        WaypointComponent(const EntityID p, const EntityID n) : prev(p), next(n) {}
        WaypointComponent(const WaypointComponent&) = default;
        WaypointComponent(WaypointComponent&&) = default;
        WaypointComponent& operator=(const WaypointComponent&) = default;
        WaypointComponent& operator=(WaypointComponent&&) = default;
    };

    template <typename ...Components>
    struct ComponentGroup {};

    using AllComponents = ComponentGroup<TransformComponent, CameraComponent, ModelComponent, ColliderComponent,
                                         PlayerCharacterComponent, AICharacterComponent, CollisionComponent, WaypointComponent>;

    class ComponentsManager {
    public:
        ComponentsManager() = default;
        ~ComponentsManager() = default;

        void init();
        void deinit();

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
        bool HasComponent(EntityID e) const { return this->GetComponentPool<T>()->Has(e); }

        template <typename T>
        const T& GetComponent(EntityID e) const { return this->GetComponentPool<T>()->Get(e); }

        template <typename T>
        T& GetComponent(EntityID e) { return this->GetComponentPool<T>()->Get(e); }

        template <typename T, typename ...Args>
        void AddComponent(EntityID e, Args&& ...args) {
            this->GetComponentPool<T>()->Add(e, std::forward<Args>(args) ...);
        }

        template <typename T>
        void RemoveComponent(EntityID e) { this->GetComponentPool<T>()->Remove(e); }

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
