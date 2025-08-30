#pragma once

#include "entity.h"
#include "component.h"
#include "system.h"
#include "pool.h"

#include <set>

namespace Ecs {

	class World {
	public:
		EntityID CreateEntity() {
			auto res = m_entitiesManager.CreateEntity();
			m_entities.insert(res);
			return res;
		}

		void DestroyEntity(const EntityID id) {
			auto e_sig = m_entitiesManager.GetSignature(id);

			for (auto& [_, p] : m_componentsManager.GetComponentPools()) {
				p.Remove(id);
			}

			m_entities.erase(id);
			m_entitiesManager.DestroyEntity(id);
		}

		template <typename T>
		bool HasComponent(EntityID id) const {
			return m_componentsManager.HasComponent(id);
		}

		template <typename T>
		const T& GetComponent(EntityID id) const {
			return m_componentsManager.GetComponent(id);
		}

		template <typename T>
		T& GetComponent(EntityID id) {
			return m_componentsManager.GetComponent(id);
		}

		template <typename T, ComponentTypes Type, typename ...Args>
		void AddComponent(EntityID id, Args&& ...args) {
			m_componentsManager.AddComponent(id, std::forward<Args>(args)...);
			auto e_sig = m_entitiesManager.GetSignature(id);
			m_entitiesManager.SetSignature(id, e_sig.set(Type, true));
		}

		template <typename T>
		bool RemoveComponent(EntityID id) {
			return m_componentsManager.RemoveComponent(id);
		}

		void Start();
		void Update(double dt);

	private:
		std::set<EntityID> m_entities;
		EntityManager m_entitiesManager;
		ComponentsManager m_componentsManager;
		SystemsManager m_systemsManager;
	};

} // namespace Ecs
