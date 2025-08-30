#include "config.h"
#include "system.h"

namespace Ecs {

	SystemsManager::SystemsManager() {
		RegisterSystems(AllSystems{});

		for (const auto& [k, v] : this->m_systems) {
			std::cout << k << ' ' << typeid(v).name() << '\n';
		}
	}

	SystemsManager::~SystemsManager() {
		for (auto& [_, v] : this->m_systems) {
			delete v;
		}
	}

} // namespace Ecs