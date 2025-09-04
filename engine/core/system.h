#pragma once

#include "ecs_types.h"
#include "component.h"

#include <unordered_map>

#include <iostream>


namespace Ecs {
    struct BaseSystem {
        BaseSystem()
            : sig() {}

        BaseSystem(Signature sig)
            : sig(sig) {}

        virtual ~BaseSystem() {}

        virtual void Start(const std::vector<EntityID>& entities) {}
        virtual void Update(const std::vector<EntityID>& entities, double dt) {}
        virtual void Draw(const std::vector<EntityID>& entities) {}

        Signature sig;
    };

    template <ComponentTypes ...T>
    struct BaseSystemInt : public BaseSystem {
        BaseSystemInt()
            : BaseSystem() { ([&] { sig |= T; }(), ...); }

        virtual ~BaseSystemInt() override {}
    };

    struct RigidBodySystem : public BaseSystemInt<CT_TRANSFORM, CT_CAMERA> {
        RigidBodySystem()
            : BaseSystemInt() {}

        virtual void Start(const std::vector<EntityID>& entities) override {
            for (auto e: entities) { std::cout << "RigidBodySystem::Start for entity: " << e << '\n'; }
        }

        virtual void Update(const std::vector<EntityID>& entities, double dt) override {
            for (auto e: entities) { std::cout << "RigidBodySystem::Update for entity: " << e << '\n'; }
        }
    };

    struct DrawableSystem : public BaseSystemInt<CT_TRANSFORM, CT_MESH> {
        DrawableSystem()
            : BaseSystemInt() {}

        virtual void Start(const std::vector<EntityID>& entities) override {
            for (auto e: entities) { std::cout << "DrawableSystem::Start for entity: " << e << '\n'; }
        }

        virtual void Draw(const std::vector<EntityID>& entities) override {
            for (auto e: entities) { std::cout << "DrawableSystem::Draw for entity: " << e << '\n'; }
        }
    };

    template <typename ...Systems>
    struct SystemGroup {};

    using AllSystems = SystemGroup<RigidBodySystem, DrawableSystem>;

    class SystemsManager {
    public:
        using iterator = std::unordered_map<const char*, BaseSystem*>::iterator;
        using const_iterator = std::unordered_map<const char*, BaseSystem*>::const_iterator;

        SystemsManager();
        ~SystemsManager();

        iterator begin() { return this->m_systems.begin(); }
        iterator end() { return this->m_systems.end(); }
        const_iterator begin() const { return this->m_systems.begin(); }
        const_iterator end() const { return this->m_systems.end(); }

    private:
        std::unordered_map<const char*, BaseSystem*> m_systems;

        template <typename ...System>
        void RegisterSystems(SystemGroup<System ...>) {
            ([&]() {
                auto tname = typeid(System).name();

                this->m_systems[tname] = reinterpret_cast<BaseSystem*>(new System{});
            }(), ...);
        }
    };
} // namespace Ecs
