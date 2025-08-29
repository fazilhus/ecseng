#pragma once

#include "ecs_types.h"
#include "pool.h"

#include <unordered_map>

namespace Ecs {

	struct TransformComponent {};

	struct CameraComponent {};

	template<typename ...Components>
	struct ComponentGroup {

	};

	using AllComponents = ComponentGroup<TransformComponent, CameraComponent>;

	class ComponentsManager {
	public:
		ComponentsManager();
		~ComponentsManager();

		template <typename T>
		void RegisterComponent() {
			auto tname = typeid(T).name();

			assert(this->m_componentTypes.find(tname) == this->m_componentTypes.end() && "component already registered");

			this->m_componentTypes[tname] = this->m_nextComponentType++;
			this->m_components[tname] = ComponentPool<T>();
		}
	
	private:
		std::unordered_map<const char*, ComponentID> m_componentTypes;
		std::unordered_map<const char*, BaseComponentPool> m_components;
		ComponentID m_nextComponentType;
	};

} // namespace Ecs
