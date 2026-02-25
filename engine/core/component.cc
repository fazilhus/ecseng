#include "config.h"
#include "component.h"

#include <iostream>
#include <string>


namespace Ecs {
    ComponentsManager::ComponentsManager()
        : m_nextComponentType(1) {
        RegisterComponents(AllComponents{});

        for (const auto& [k, v]: this->m_componentTypes) { std::cout << k.name() << ' ' << std::to_string(v) << '\n'; }
    }

    ComponentsManager::~ComponentsManager() { for (auto& [k, v]: m_components) { delete v; } }
} // namespace Ecs
