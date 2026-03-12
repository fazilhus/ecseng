#include "config.h"
#include "system.h"
#include "world.h"
#include "render/cameramanager.h"
#include "render/debugrender.h"
#include "render/input/inputserver.h"

#include <gtx/vector_angle.hpp>

#include "random.h"


namespace Ecs {

    void PhysicsBodySystem::PhysicsUpdate(const std::vector<EntityID>& entities, float dt) {
        for (auto e : entities) {
            auto& t_comp = world->GetComponent<TransformComponent>(e);
            const auto& c_comp = world->GetComponent<CollisionComponent>(e);
            for (const auto& v : c_comp.rays) {
                const auto dir = glm::vec3(t_comp.transform * glm::vec4(glm::normalize(v), 0.0f));
                const auto len = glm::length(v);
                Physics::RaycastPayload payload = Physics::Raycast(t_comp.pos, dir, len);
#if 0
// #if _DEBUG
                Debug::DrawLine(
                    t_comp.pos, t_comp.pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1),
                    Debug::RenderMode::AlwaysOnTop
            );
#endif

                if (payload.hit) {
                    const auto waypoints = world->GetAllEntitiesBySignature(CT_WAYPOINT);
                    const auto wp = waypoints[Core::FastRandom() % waypoints.size()];
                    const auto& wp_t_comp = world->GetComponent<TransformComponent>(wp);
                    if (world->HasComponent<AICharacterComponent>(e)) {
                        auto& ai_comp = world->GetComponent<AICharacterComponent>(e);
                        const auto& wp_wp_cp = world->GetComponent<WaypointComponent>(wp);
                        ai_comp.heading = wp_wp_cp.next;
                    }
                    t_comp.pos = wp_t_comp.pos;
                    t_comp.rot = glm::identity<glm::quat>();
                    t_comp.transform = glm::translate(t_comp.pos) * glm::mat4_cast(t_comp.rot) * glm::scale(t_comp.scale);
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

    void PlayerControllerSystem::Start(const std::vector<EntityID>& entities) {
        auto& cam_comp = world->GetComponent<CameraComponent>(entities[0]);
        cam_comp.cam_pos = glm::vec3(0, 1.0f, -2.0f);
        cam_comp.cam_offset = glm::vec3(0.0f, 1.0f, -4.0f);
        cam_comp.cam_smooth = 10.0f;
    }

    void PlayerControllerSystem::Update(const std::vector<EntityID>& entities, float dt) {
        using namespace Input;
        using namespace Render;
        const Keyboard* kbd = GetDefaultKeyboard();

        auto& t_comp = world->GetComponent<TransformComponent>(entities[0]);
        auto& cam_comp = world->GetComponent<CameraComponent>(entities[0]);
        const auto& char_comp = world->GetComponent<PlayerCharacterComponent>(entities[0]);
        auto& mov_comp = world->GetComponent<MovementComponent>(entities[0]);

        if (kbd->held[Key::W]) {
            if (kbd->held[Key::Shift])
                mov_comp.currentSpeed = glm::mix(mov_comp.currentSpeed, mov_comp.boostSpeed, std::min(1.0f, dt * 30.0f));
            else
                mov_comp.currentSpeed = glm::mix(mov_comp.currentSpeed, mov_comp.normalSpeed, std::min(1.0f, dt * 90.0f));
        }
        else { mov_comp.currentSpeed = 0; }
        const glm::vec3 desiredVelocity = t_comp.transform * glm::vec4(0, 0, mov_comp.currentSpeed, 0.0f);

        mov_comp.linearVelocity = glm::mix(mov_comp.linearVelocity, desiredVelocity, dt * mov_comp.accelerationFactor);

        const float rotX = kbd->held[Key::Left] ? 1.0f : kbd->held[Key::Right] ? -1.0f : 0.0f;
        const float rotY = kbd->held[Key::Up] ? -1.0f : kbd->held[Key::Down] ? 1.0f : 0.0f;
        const float rotZ = kbd->held[Key::A] ? -1.0f : kbd->held[Key::D] ? 1.0f : 0.0f;

        t_comp.pos += mov_comp.linearVelocity * dt * 10.0f;

        const float rotationSpeed = 1.8f * dt;
        mov_comp.rotXSmooth = glm::mix(mov_comp.rotXSmooth, rotX * rotationSpeed, dt * cam_comp.cam_smooth);
        mov_comp.rotYSmooth = glm::mix(mov_comp.rotYSmooth, rotY * rotationSpeed, dt * cam_comp.cam_smooth);
        mov_comp.rotZSmooth = glm::mix(mov_comp.rotZSmooth, rotZ * rotationSpeed, dt * cam_comp.cam_smooth);
        const auto localOrientation = glm::quat(glm::vec3(-mov_comp.rotYSmooth, mov_comp.rotXSmooth, mov_comp.rotZSmooth));
        t_comp.rot *= localOrientation;
        mov_comp.rotationZ -= mov_comp.rotXSmooth;
        mov_comp.rotationZ = glm::clamp(mov_comp.rotationZ, -45.0f, 45.0f);
        t_comp.transform = glm::translate(t_comp.pos) * glm::mat4_cast(glm::quat(t_comp.rot)) * glm::mat4_cast(glm::quat(glm::vec3(0, 0, mov_comp.rotationZ)));
        mov_comp.rotationZ = glm::mix(mov_comp.rotationZ, 0.0f, dt * cam_comp.cam_smooth);

        // update camera view transform
        const glm::vec3 desiredCamPos = t_comp.pos + glm::vec3(t_comp.transform * glm::vec4(cam_comp.cam_offset, 0));
        cam_comp.cam_pos = glm::mix(cam_comp.cam_pos, desiredCamPos, dt * cam_comp.cam_smooth);
        cam_comp.view = lookAt(cam_comp.cam_pos, cam_comp.cam_pos + glm::vec3(t_comp.transform[2]), glm::vec3(t_comp.transform[1]));
    }

    void PlayerControllerSystem::BeforeDraw(const std::vector<EntityID>& entities) {
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

    glm::quat look_at(const glm::vec3& dir, const glm::vec3& up) {
        glm::mat3 res{};

        res[2] = dir;
        res[0] = normalize(cross(up, res[2]));
        res[1] = cross(res[2], res[0]);

        return glm::quat_cast(res);
    }

    void AIControllerSystem::Update(const std::vector<EntityID>& entities, float dt) {
        for (const auto e : entities) {
            auto& t_comp = world->GetComponent<TransformComponent>(e);
            auto& ch_comp = world->GetComponent<AICharacterComponent>(e);
            auto& mov_comp = world->GetComponent<MovementComponent>(e);

            glm::vec3 target{};
            glm::vec3 dir{};
            const auto players = world->GetAllEntitiesByComponentIDs<CT_PLAYER_CHARACTER>();
            auto min_player_dist{1e30f};
            for (auto p : players) {
                const auto& p_t_comp = world->GetComponent<TransformComponent>(p);
                const auto new_dist = glm::distance(t_comp.pos, p_t_comp.pos);
                if (new_dist < min_player_dist) {
                    min_player_dist = new_dist;
                    target = p_t_comp.pos;
                }
            }

            switch (ch_comp.behaviour) {
            case BT_Neutral:
            case BT_Aggressive: {
                if (ch_comp.range > min_player_dist) {
                    ch_comp.state = ST_Acting;
                } else {
                    ch_comp.state = ST_Moving;
                }
            } break;
            case BT_Defensive: {
                if (ch_comp.range > min_player_dist) {
                    ch_comp.state = ST_Acting;
                    target *= -1.0f;
                } else {
                    ch_comp.state = ST_Moving;
                }
            } break;
            }

            switch (ch_comp.state) {
            case ST_Moving: {
                target = world->GetComponent<TransformComponent>(ch_comp.heading).pos;
                dir = target - t_comp.pos;
                if (glm::length(dir) < 2.5f) {
                    ch_comp.heading = world->GetComponent<WaypointComponent>(ch_comp.heading).next;
                    target = world->GetComponent<TransformComponent>(ch_comp.heading).pos;
                    dir = target - t_comp.pos;
                }
            } break;
            case ST_Acting: {
                dir = target - t_comp.pos;
            } break;
            }

            dir = glm::normalize(dir);
            const auto target_rot = look_at(dir, glm::vec3(0.0f, 1.0f, 0.0f));

            const glm::vec3 desiredVelocity = t_comp.transform * glm::vec4(0, 0, mov_comp.normalSpeed, 0.0f);
            mov_comp.linearVelocity = glm::mix(mov_comp.linearVelocity, desiredVelocity, dt * mov_comp.accelerationFactor);
            t_comp.pos += mov_comp.linearVelocity * 10.0f * dt;

            t_comp.rot = glm::normalize(glm::slerp(t_comp.rot, target_rot, 10.0f * dt));
            t_comp.transform = glm::translate(t_comp.pos) * glm::mat4_cast(glm::quat(t_comp.rot)) * glm::scale(t_comp.scale);
        }
    }

    void ParticleSystem::BeforeDraw(const std::vector<EntityID>& entities) {
        for (auto e : entities) {
            const auto& t_comp = world->GetComponent<TransformComponent>(e);
            auto& pe_comp = world->GetComponent<ParticleEmitterComponent>(e);
            const auto& mov_comp = world->GetComponent<MovementComponent>(e);

            pe_comp.emitter.data.origin = glm::vec4(t_comp.pos + glm::vec3(t_comp.transform[2]) * pe_comp.offset, 1);
            pe_comp.emitter.data.dir = glm::vec4(glm::vec3(-t_comp.transform[2]), 0);

            const auto t = mov_comp.currentSpeed / mov_comp.normalSpeed;
            pe_comp.emitter.data.startSpeed = 1.2f + (3.0f * t);
            pe_comp.emitter.data.endSpeed = 0.0f + (3.0f * t);
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
