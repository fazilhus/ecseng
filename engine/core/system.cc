#include "config.h"
#include "system.h"
#include "world.h"

namespace Ecs {

    void DrawableSystem::Draw(const std::vector<EntityID>& entities) {
        for (const auto e : entities) {
            const auto& transform = world->GetComponent<TransformComponent>(e);
            const auto& model = world->GetComponent<ModelComponent>(e);
            Render::RenderDevice::Draw(model.model_id, transform.get_transform());
        }
    }

    void SystemsManager::init(World* w) {
        RegisterSystems(w, AllSystems{});

        // for (const auto& [k, v]: this->m_systems) { std::cout << k.name() << ' ' << typeid(v).name() << '\n'; }
    }

    void SystemsManager::deinit() {
        for (auto& [_, v]: this->m_systems) { delete v; }
    }

} // namespace Ecs
