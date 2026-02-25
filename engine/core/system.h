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
        }

        virtual void Update(const std::vector<EntityID>& entities, double dt) override {
        }
    };

    struct DrawableSystem : public BaseSystemInt<CT_TRANSFORM, CT_MODEL> {
        DrawableSystem()
            : BaseSystemInt() {}

        virtual void Start(const std::vector<EntityID>& entities) override {
        }

        virtual void Draw(const std::vector<EntityID>& entities) override {
        }
    };

    template <typename ...Systems>
    struct SystemGroup {};

    using AllSystems = SystemGroup<RigidBodySystem, DrawableSystem>;

    class SystemsManager {
    public:
        using systems = std::unordered_map<std::type_index, BaseSystem*>;
        using iterator = systems::iterator;
        using const_iterator = systems::const_iterator;

        SystemsManager();
        ~SystemsManager();

        iterator begin() { return this->m_systems.begin(); }
        iterator end() { return this->m_systems.end(); }
        const_iterator begin() const { return this->m_systems.begin(); }
        const_iterator end() const { return this->m_systems.end(); }

    private:
        systems m_systems;

        template <typename ...System>
        void RegisterSystems(SystemGroup<System ...>) {
            ([&]() {
                this->m_systems[std::type_index(typeid(System))] = reinterpret_cast<BaseSystem*>(new System{});
            }(), ...);
        }
    };
} // namespace Ecs
