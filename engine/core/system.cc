#include "config.h"
#include "system.h"
#include "world.h"
#include "render/cameramanager.h"
#include "render/debugrender.h"
#include "render/input/inputserver.h"


namespace Ecs {

    void PhysicsBodySystem::PhysicsUpdate(const std::vector<EntityID>& entities, float dt) {
        for (auto e : entities) {
            const auto& t_comp = world->GetComponent<TransformComponent>(e);
            const auto& c_comp = world->GetComponent<CollisionComponent>(e);
            for (const auto& v : c_comp.rays) {
                const auto dir = glm::vec3(t_comp.transform * glm::vec4(glm::normalize(v), 0.0f));
                const auto len = glm::length(v);
                Physics::RaycastPayload payload = Physics::Raycast(t_comp.pos, dir, len);

#if _DEBUG
                Debug::DrawLine(
                    t_comp.pos, t_comp.pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1),
                    Debug::RenderMode::AlwaysOnTop
            );
#endif

                if (payload.hit) {
                    Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                }
            }
        }
    }

    void DrawableSystem::Draw(const std::vector<EntityID>& entities) {
        for (const auto e : entities) {
            const auto& transform = world->GetComponent<TransformComponent>(e);
            const auto& model = world->GetComponent<ModelComponent>(e);
            Render::RenderDevice::Draw(model.model_id, transform.transform);
        }
    }

    void PlayableSystem::Start(const std::vector<EntityID>& entities) {
        auto& cam_comp = world->GetComponent<CameraComponent>(entities[0]);
        cam_comp.cam_pos = glm::vec3(0, 1.0f, -2.0f);
        cam_comp.cam_offset = glm::vec3(0.0f, 1.0f, -4.0f);
        cam_comp.cam_smooth = 10.0f;
    }

    void PlayableSystem::Update(const std::vector<EntityID>& entities, float dt) {
        using namespace Input;
        using namespace Render;
        const Keyboard* kbd = GetDefaultKeyboard();

        // Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);

        auto& t_comp = world->GetComponent<TransformComponent>(entities[0]);
        auto& cam_comp = world->GetComponent<CameraComponent>(entities[0]);
        auto& char_comp = world->GetComponent<CharacterComponent>(entities[0]);

        if (kbd->held[Key::W]) {
            if (kbd->held[Key::Shift])
                char_comp.currentSpeed = glm::mix(char_comp.currentSpeed, char_comp.boostSpeed, std::min(1.0f, dt * 30.0f));
            else
                char_comp.currentSpeed = glm::mix(char_comp.currentSpeed, char_comp.normalSpeed, std::min(1.0f, dt * 90.0f));
        }
        else { char_comp.currentSpeed = 0; }
        auto desiredVelocity = glm::vec3(0, 0, char_comp.currentSpeed);
        desiredVelocity = t_comp.transform * glm::vec4(desiredVelocity, 0.0f);

        char_comp.linearVelocity = glm::mix(char_comp.linearVelocity, desiredVelocity, dt * char_comp.accelerationFactor);

        const float rotX = kbd->held[Key::Left] ? 1.0f : kbd->held[Key::Right] ? -1.0f : 0.0f;
        const float rotY = kbd->held[Key::Up] ? -1.0f : kbd->held[Key::Down] ? 1.0f : 0.0f;
        const float rotZ = kbd->held[Key::A] ? -1.0f : kbd->held[Key::D] ? 1.0f : 0.0f;

        t_comp.pos += char_comp.linearVelocity * dt * 10.0f;

        const float rotationSpeed = 1.8f * dt;
        char_comp.rotXSmooth = glm::mix(char_comp.rotXSmooth, rotX * rotationSpeed, dt * cam_comp.cam_smooth);
        char_comp.rotYSmooth = glm::mix(char_comp.rotYSmooth, rotY * rotationSpeed, dt * cam_comp.cam_smooth);
        char_comp.rotZSmooth = glm::mix(char_comp.rotZSmooth, rotZ * rotationSpeed, dt * cam_comp.cam_smooth);
        const auto localOrientation = glm::quat(glm::vec3(-char_comp.rotYSmooth, char_comp.rotXSmooth, char_comp.rotZSmooth));
        t_comp.rot *= localOrientation;
        char_comp.rotationZ -= char_comp.rotXSmooth;
        char_comp.rotationZ = glm::clamp(char_comp.rotationZ, -45.0f, 45.0f);
        t_comp.transform = glm::translate(t_comp.pos) * glm::mat4_cast(glm::quat(t_comp.rot)) * glm::mat4_cast(glm::quat(glm::vec3(0, 0, char_comp.rotationZ)));
        char_comp.rotationZ = glm::mix(char_comp.rotationZ, 0.0f, dt * cam_comp.cam_smooth);

        // update camera view transform
        const glm::vec3 desiredCamPos = t_comp.pos + glm::vec3(t_comp.transform * glm::vec4(cam_comp.cam_offset, 0));
        cam_comp.cam_pos = glm::mix(cam_comp.cam_pos, desiredCamPos, dt * cam_comp.cam_smooth);
        cam_comp.view = lookAt(cam_comp.cam_pos, cam_comp.cam_pos + glm::vec3(t_comp.transform[2]), glm::vec3(t_comp.transform[1]));
    }

    void PlayableSystem::BeforeDraw(const std::vector<EntityID>& entities) {
        using namespace Render;
        using namespace glm;
        const auto& cam_comp = world->GetComponent<CameraComponent>(entities[0]);
        const auto main_cam = CameraManager::GetCamera(CAMERA_MAIN);
        main_cam->view = cam_comp.view;
        main_cam->projection = cam_comp.projection;
        main_cam->invView = inverse(cam_comp.view);
        main_cam->invProjection = inverse(cam_comp.projection);
        main_cam->viewProjection = cam_comp.projection * cam_comp.view;
        main_cam->invViewProjection = inverse(main_cam->viewProjection);
    }

    void SystemsManager::init(World* w) {
        RegisterSystems(w, AllSystems{});

        // for (const auto& [k, v]: this->m_systems) { std::cout << k.name() << ' ' << typeid(v).name() << '\n'; }
    }

    void SystemsManager::deinit() {
        for (auto& [_, v]: this->m_systems) { delete v; }
    }

} // namespace Ecs
