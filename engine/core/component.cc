#include "config.h"
#include "component.h"

#include <iostream>
#include <string>


namespace Ecs {

    CameraComponent::CameraComponent(const glm::mat4& v, const glm::mat4& p) {
        this->view = v;
        this->projection = p;
        this->invView = glm::inverse(v);
        this->invProjection = glm::inverse(p);
        this->viewProjection = p * v;
        this->invViewProjection = glm::inverse(this->viewProjection);
    }

    void ComponentsManager::init() {
        m_nextComponentType = 1;
        RegisterComponents(AllComponents{});
    }

    void ComponentsManager::deinit() {
        for (auto& [k, v]: m_components) { delete v; }
    }

} // namespace Ecs
