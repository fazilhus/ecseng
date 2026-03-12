#include "config.h"
#include "component.h"

#include "render/particlesystem.h"


namespace Ecs {

    TransformComponent::TransformComponent(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
            : pos(pos), rot(rot), scale(scale) {
        this->transform = glm::translate(this->pos) * glm::mat4_cast(this->rot) * glm::scale(this->scale);
    }

    CameraComponent::CameraComponent(const glm::mat4& v, const glm::mat4& p) {
        this->view = v;
        this->projection = p;
    }

    AICharacterComponent::AICharacterComponent(const EntityID h, const BehaviourType b) : heading(h), behaviour(b) {
        switch (behaviour) {
        case BT_Aggressive: {
            range = 15.0f;
        } break;
        case BT_Neutral: {
            range = 15.0f;
        } break;
        case BT_Defensive: {
            range = 15.0f;
        } break;
        }
    }

    ParticleEmitterComponent::ParticleEmitterComponent(const float zo, const glm::vec4& color) : offset(zo) {
        constexpr uint32_t numParticles = 2048;
        emitter.init(numParticles);
        emitter.data = {
            .origin = glm::vec4(glm::vec3(0.0f, 0.0f, zo), 1.0f),
            .dir = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
            .startColor = color * 2.0f,
            .endColor = glm::vec4(0, 0, 0, 1.0f),
            .numParticles = numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.025f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };
        Render::ParticleSystem::Instance()->AddEmitter(&emitter);
    }

    void ComponentsManager::init() {
        m_nextComponentType = 1;
        RegisterComponents(AllComponents{});
    }

    void ComponentsManager::deinit() {
        for (auto& [k, v]: m_components) { delete v; }
    }

} // namespace Ecs
