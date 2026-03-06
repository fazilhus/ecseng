#pragma once

#include "ecs_types.h"
#include "component.h"

#include <unordered_map>

#include <iostream>


namespace Ecs {
    class World;

    struct BaseSystem {
        BaseSystem(World* w)
            : sig(), world(w) {}

        // BaseSystem(World* w, Signature sig)
        //     : sig(sig), world(w) {}

        virtual ~BaseSystem() {}

        virtual void Start(const std::vector<EntityID>& entities) {}
        virtual void Update(const std::vector<EntityID>& entities, float dt) {}
        virtual void BeforeDraw(const std::vector<EntityID>& entities) {}
        virtual void Draw(const std::vector<EntityID>& entities) {}

        Signature sig;
        World* world;
    };

    template <ComponentTypes ...T>
    struct BaseSystemInt : public BaseSystem {
        BaseSystemInt(World* w)
            : BaseSystem(w) { ([&] { sig |= T; }(), ...); }
        virtual ~BaseSystemInt() override {}
    };

    struct RigidBodySystem final : public BaseSystemInt<CT_TRANSFORM, CT_CAMERA> {
        RigidBodySystem(World* w)
            : BaseSystemInt(w) {}
    };

    struct DrawableSystem final : public BaseSystemInt<CT_TRANSFORM, CT_MODEL> {
        DrawableSystem(World* w)
            : BaseSystemInt(w) {}
        virtual void Draw(const std::vector<EntityID>& entities) override;
    };

    struct PlayableSystem final : public BaseSystemInt<CT_TRANSFORM, CT_CAMERA, CT_CHARACTER> {
        PlayableSystem(World* w)
            : BaseSystemInt(w) {}
        virtual void Start(const std::vector<EntityID>& entities) override;
        virtual void Update(const std::vector<EntityID>& entities, float dt) override;
        virtual void BeforeDraw(const std::vector<EntityID>& entities) override;
    };

    template <typename ...Systems>
    struct SystemGroup {};

    using AllSystems = SystemGroup<RigidBodySystem, DrawableSystem, PlayableSystem>;

    class SystemsManager {
    public:
        using systems = std::unordered_map<std::type_index, BaseSystem*>;
        using iterator = systems::iterator;
        using const_iterator = systems::const_iterator;

        SystemsManager() = default;
        ~SystemsManager() = default;

        void init(World* w);
        void deinit();

        iterator begin() { return this->m_systems.begin(); }
        iterator end() { return this->m_systems.end(); }
        const_iterator begin() const { return this->m_systems.begin(); }
        const_iterator end() const { return this->m_systems.end(); }

    private:
        systems m_systems;

        template <typename ...System>
        void RegisterSystems(World* w, SystemGroup<System ...>) {
            ([&]() {
                this->m_systems[std::type_index(typeid(System))] = reinterpret_cast<BaseSystem*>(new System(w));
            }(), ...);
        }
    };
} // namespace Ecs
