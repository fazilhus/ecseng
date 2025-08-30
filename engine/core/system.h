#pragma once

#include "ecs_types.h"
#include "component.h"

#include <unordered_map>

#include <iostream>

namespace Ecs {

	struct BaseSystem {
		virtual ~BaseSystem() {}
	};

	template <ComponentTypes ...T>
	struct BaseSystemInt {
		BaseSystemInt() : sig() {
			([&]
				{
					sig |= T;
				} (), ...);
		}
		virtual ~BaseSystemInt() {}

		virtual void Start(double dt) = 0;
		virtual void Update() = 0;

		Signature sig;
	};

	struct RigidBodySystem : public BaseSystemInt<CT_TRANSFORM, CT_CAMERA> {
		RigidBodySystem() : BaseSystemInt() {}

		virtual void Start(double dt) override {}
		virtual void Update() override {}
	};

	template<typename ...Systems>
	struct SystemGroup {

	};

	using AllSystems = SystemGroup<RigidBodySystem>;

	class SystemsManager {
	public:
		SystemsManager();
		~SystemsManager();

	private:
		std::unordered_map<const char*, BaseSystem*> m_systems;

		template <typename ...System>
		void RegisterSystems(SystemGroup<System...>) {
			([&]() {
				auto tname = typeid(System).name();

				this->m_systems[tname] = reinterpret_cast<BaseSystem*>(new System{});
				}(), ...);
		}
	};

} // namespace Ecs