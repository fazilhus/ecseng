#include "config.h"
#include "component.h"

#include <iostream>
#include <string>


namespace Ecs {

    TransformComponent::TransformComponent(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
            : pos(pos), rot(rot), scale(scale) {
        this->transform = glm::translate(this->pos) * glm::mat4_cast(this->rot) * glm::scale(this->scale);
    }

    CameraComponent::CameraComponent(const glm::mat4& v, const glm::mat4& p) {
        this->view = v;
        this->projection = p;
    }

    void ComponentsManager::init() {
        m_nextComponentType = 1;
        RegisterComponents(AllComponents{});
    }

    void ComponentsManager::deinit() {
        for (auto& [k, v]: m_components) { delete v; }
    }

} // namespace Ecs
