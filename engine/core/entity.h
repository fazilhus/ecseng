#pragma once

#include "ecs_types.h"

#include <queue>

namespace Ecs {

	class EntityManager {
	public:
		EntityManager();
		~EntityManager();

		EntityID CreateEntity();
		void DestroyEntity(EntityID id);

		Signature GetSignature(EntityID id) const;
		void SetSignature(EntityID id, Signature sig);

	private:
		std::priority_queue<EntityID> m_freeEntities;
		std::vector<Signature> m_signatures;
	};

} // namespace Ecs