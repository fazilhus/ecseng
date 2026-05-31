//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"

#include <array>
#include <cstring>
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include <vector>
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "render/input/inputserver.h"
#include "core/cvar.h"
#include "render/physics.h"
#include <chrono>
#include <iostream>

#include "spaceship.h"
#include "gtx/quaternion.hpp"

using namespace Display;
using namespace Render;


namespace Game {
    //------------------------------------------------------------------------------
    /**
    */
    SpaceGameApp::SpaceGameApp()
        : window(nullptr), world(nullptr), ip(Core::octets_into_ip({127,0,0,1})), port(6969) {
        this->peer.init();
    }

    //------------------------------------------------------------------------------
    /**
    */
    SpaceGameApp::~SpaceGameApp() {
        this->peer.deinit();
        if (this->m_server_thread.joinable()) {
            this->m_server_stop = true;
            this->m_server_thread.join();
        }
        this->server.deinit();
    }

    //------------------------------------------------------------------------------
    /**
    */
    bool SpaceGameApp::Open() {
        App::Open();
        this->window = new Display::Window;
        this->window->SetSize(1280, 720);

        if (this->window->Open()) {
            // set clear color to gray
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            RenderDevice::Init();

            // set ui rendering function
            this->window->SetUiRender([this]() { this->RenderUI(); });

            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    /**
    */
    void SpaceGameApp::Run() {
        int w;
        int h;
        this->window->GetSize(w, h);

        this->world = new Ecs::World{};

        glm::mat4 projection = glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
        Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
        cam->projection = projection;

        // load all resources
        ModelId models[6] = {
            LoadModel("assets/space/Asteroid_1.glb"),
            LoadModel("assets/space/Asteroid_2.glb"),
            LoadModel("assets/space/Asteroid_3.glb"),
            LoadModel("assets/space/Asteroid_4.glb"),
            LoadModel("assets/space/Asteroid_5.glb"),
            LoadModel("assets/space/Asteroid_6.glb")
        };
        ModelId laser_proj_model = LoadModel("assets/space/laser.glb");

        Physics::ColliderMeshId colliderMeshes[6] = {
            Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
        };
        Physics::ColliderMeshId laser_proj_cmesh = Physics::LoadColliderMesh("assets/space/laser.glb");

        std::vector<Ecs::EntityID> asteroids;

        // Setup asteroids near
        for (int i = 0; i < 100; i++) {
            const auto resourceIndex = static_cast<size_t>(Core::FastRandom() % 6);
            constexpr auto span = 30.0f;
            const auto translation = glm::vec3(
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span
            );
            const auto rotationAxis = glm::normalize(translation);
            const auto rotation = glm::quat(Core::RandomFloatNTP(), rotationAxis);
            const auto e = world->CreateEntity();
            asteroids.push_back(e);
            world->AddComponent<Ecs::ModelComponent, Ecs::CT_MODEL>(e, models[resourceIndex]);
            world->AddComponent<Ecs::ColliderComponent, Ecs::CT_COLLIDER>(e, colliderMeshes[resourceIndex], true);
            world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(e, translation, rotation, glm::vec3(1.0f));
        }

        // Setup asteroids far
        for (int i = 0; i < 50; i++) {
            const auto resourceIndex = static_cast<size_t>(Core::FastRandom() % 6);
            constexpr auto span = 100.0f;
            const auto translation = glm::vec3(
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span
            );
            const auto rotationAxis = glm::normalize(translation);
            const auto rotation = glm::quat(Core::RandomFloatNTP(), rotationAxis);
            const auto e = world->CreateEntity();
            asteroids.push_back(e);
            world->AddComponent<Ecs::ModelComponent, Ecs::CT_MODEL>(e, models[resourceIndex]);
            world->AddComponent<Ecs::ColliderComponent, Ecs::CT_COLLIDER>(e, colliderMeshes[resourceIndex], true);
            world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(e, translation, rotation, glm::vec3(1.0f));
        }

        std::vector<Ecs::EntityID> waypoints;
        for (auto i = 0; i < 4; ++i ) {
            const auto e = world->CreateEntity();
            waypoints.push_back(e);
        }
        {
            const auto zero_rot = glm::quat_cast(glm::identity<glm::mat4>());
            world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[0], glm::vec3(-10.0f, 0.0f, -10.0f), zero_rot, glm::vec3(1.0f));
            world->AddComponent<Ecs::WaypointComponent, Ecs::CT_WAYPOINT>(waypoints[0], waypoints[3], waypoints[1]);

            world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[1], glm::vec3(-10.0f, 0.0f, 10.0f), zero_rot, glm::vec3(1.0f));
            world->AddComponent<Ecs::WaypointComponent, Ecs::CT_WAYPOINT>(waypoints[1], waypoints[0], waypoints[2]);

            world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[2], glm::vec3(10.0f, 0.0f, 10.0f), zero_rot, glm::vec3(1.0f));
            world->AddComponent<Ecs::WaypointComponent, Ecs::CT_WAYPOINT>(waypoints[2], waypoints[1], waypoints[3]);

            world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[3], glm::vec3(10.0f, 0.0f, -10.0f), zero_rot, glm::vec3(1.0f));
            world->AddComponent<Ecs::WaypointComponent, Ecs::CT_WAYPOINT>(waypoints[3], waypoints[2], waypoints[0]);
        }

        // Setup skybox
        std::vector<const char*> skybox
        {
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png"
        };
        TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
        RenderDevice::SetSkybox(skyboxId);

        Input::Keyboard* kbd = Input::GetDefaultKeyboard();

        const int numLights = 40;
        Render::PointLightId lights[numLights];
        // Setup lights
        for (int i = 0; i < numLights; i++) {
            glm::vec3 translation = glm::vec3(
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f
            );
            glm::vec3 color = glm::vec3(
                Core::RandomFloat(),
                Core::RandomFloat(),
                Core::RandomFloat()
            );
            lights[i] = Render::LightServer::CreatePointLight(
                translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f)
            );
        }

        // SpaceShip ship;
        const auto ship_model = LoadModel("assets/space/spaceship.glb");
        const auto ship_collider = std::vector{
            glm::vec3(1.40173, 0.0, -0.225342), // left wing back
            glm::vec3(1.33578, 0.0, 0.088893), // left wing front
            glm::vec3(0.227107, -0.200232, -0.588618), // left back engine bottom
            glm::vec3(0.227107, 0.228809, -0.588618), // left back engine top
            glm::vec3(0.391073, -0.130853, 1.28339), // left weapon
            glm::vec3(0.134787, 0.0, 1.68965), // left front
            glm::vec3(0.134787, 0.250728, 0.647422), // left wind shield

            glm::vec3(-1.40173, 0.0, -0.225342), // right wing back
            glm::vec3(-1.33578, 0.0, 0.088893), // right wing front
            glm::vec3(-0.227107, -0.200232, -0.588618), // right back engine bottom
            glm::vec3(-0.227107, 0.228809, -0.588618), // right back engine top
            glm::vec3(-0.391073, -0.130853, 1.28339), // right weapon
            glm::vec3(-0.134787, 0.0, 1.68965), // right front
            glm::vec3(-0.134787, 0.250728, 0.647422), // right wind shield

            glm::vec3(0.0, 0.525049, -0.392836), // top back
            glm::vec3(0.0, 0.739624, 0.102582), // top fin
            glm::vec3(0.0, -0.244758, 0.284825) // bottom
        };

        auto ship = world->CreateEntity();
        {
            world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(ship, glm::vec3(0.0f), glm::quat(glm::radians(glm::vec3(0.0f, 90.0f, 0.0f))), glm::vec3(1.0f));
            world->AddComponent<Ecs::ModelComponent, Ecs::CT_MODEL>(ship, ship_model);
            world->AddComponent<Ecs::CameraComponent, Ecs::CT_CAMERA>(ship, glm::mat4(1.0f), glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f));
            world->AddComponent<Ecs::MovementComponent, Ecs::CT_MOVEMENT>(ship);
            world->AddComponent<Ecs::PlayerCharacterComponent, Ecs::CT_PLAYER_CHARACTER>(ship);
            world->AddComponent<Ecs::CollisionComponent, Ecs::CT_COLLISION>(ship, ship_collider);
            world->AddComponent<Ecs::ParticleEmitterComponent, Ecs::CT_PARTICLE_EMITTER>(ship, glm::vec3(0.0f, 0.0f, -0.5f), glm::vec4(0.38f, 0.76f, 0.95f, 1.0f));
            world->AddComponent<Ecs::ProjectileSpawnerComponent, Ecs::CT_PROJECTILE_SPAWNER>(ship, glm::vec3(0.0f, 0.0f, 2.0f), 30.0f, laser_proj_model, laser_proj_cmesh);
        }

        // std::vector<Ecs::EntityID> ai_ships;
        // for (auto i = 0; i < 3; ++i){
        //     const auto ai_ship = world->CreateEntity();
        //     ai_ships.push_back(ai_ship);
        //     const auto translation = glm::vec3(
        //         Core::RandomFloatNTP() * 20.0f,
        //         Core::RandomFloatNTP() * 20.0f,
        //         Core::RandomFloatNTP() * 20.0f
        //     );
        //     world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(ai_ship, translation, glm::quat(glm::mat4(1.0f)), glm::vec3(1.0f));
        //     world->AddComponent<Ecs::ModelComponent, Ecs::CT_MODEL>(ai_ship, ship_model);
        //     world->AddComponent<Ecs::MovementComponent, Ecs::CT_MOVEMENT>(ai_ship);
        //     world->AddComponent<Ecs::AICharacterComponent, Ecs::CT_AI_CHARACTER>(ai_ship, waypoints.front(), static_cast<BehaviourType>(i));
        //     world->AddComponent<Ecs::CollisionComponent, Ecs::CT_COLLISION>(ai_ship, ship_collider);
        //     world->AddComponent<Ecs::ProjectileSpawnerComponent, Ecs::CT_PROJECTILE_SPAWNER>(ai_ship, glm::vec3(0.0f, 0.0f, 2.0f), 30.0f, laser_proj_model, laser_proj_cmesh);
        //     world->AddComponent<Ecs::ParticleEmitterComponent, Ecs::CT_PARTICLE_EMITTER>(ai_ship, glm::vec3(0.0f, 0.0f, -0.5f), glm::vec4(0.1f, 0.7f, 0.1f, 1.0f));
        // }


        std::clock_t c_start = std::clock();
        auto dt = 0.01667f;

        world->Start();

        // game loop
        while (this->window->IsOpen()) {
            auto timeStart = std::chrono::steady_clock::now();
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            this->window->Update();
            peer.update();
            world->BeforeFrame();

            if (kbd->pressed[Input::Key::Code::End]) { ShaderResource::ReloadShaders(); }

            // ship.Update(dt);

            for (auto e : asteroids) {
                auto& transform_comp = world->GetComponent<Ecs::TransformComponent>(e);
                const auto& axis = glm::axis(transform_comp.rot);
                auto angle = glm::angle(transform_comp.rot);
                angle += 0.05f * dt;
                transform_comp.rot = glm::rotate(angle, axis);
                transform_comp.transform = glm::translate(transform_comp.pos) * glm::mat4_cast(transform_comp.rot);
            }

            world->PhysicsUpdate(dt);
            world->Update(dt);

            world->BeforeDraw();
            world->Draw();

            for (auto i = 0; i < 4; ++i) {
                const auto& tc = world->GetComponent<Ecs::TransformComponent>(waypoints[i]);
                Debug::DrawBox(tc.pos, tc.rot, 0.25f, glm::vec4(1.0f - 0.33f * i, 0.0f, 0.0f + 0.33f * i, 1.0f));
            }

            // Execute the entire rendering pipeline
            RenderDevice::Render(this->window, dt);

            // transfer new frame to window
            this->window->SwapBuffers();

            auto timeEnd = std::chrono::steady_clock::now();
            dt = std::min(0.04f, std::chrono::duration<float>(timeEnd - timeStart).count());

            if (kbd->pressed[Input::Key::Code::Escape])
                this->Exit();
        }
    }

    //------------------------------------------------------------------------------
    /**
    */
    void SpaceGameApp::Exit() { this->window->Close(); }

    //------------------------------------------------------------------------------
    /**
    */
    void SpaceGameApp::RenderUI() {
        if (this->window->IsOpen()) {
            ImGui::Begin("Debug");
            Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
            int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
            if (ImGui::Checkbox("Draw Light Spheres", (bool*)&drawLightSpheres))
                Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);

            Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
            int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
            if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId))
                Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);

            ImGui::Separator();

            ImGui::Text("Network");

            std::array<int, 4> octets = Core::ip_into_octets(this->ip);
            if (ImGui::InputInt4("IP Address", &octets[0])) {
                this->ip = Core::octets_into_ip(octets);
            }
            ImGui::SameLine();
            int p = this->port;
            if (ImGui::InputInt("Port", &p)) {
                this->port = p & 0xFFFF;
            }
            if (ImGui::Button("Host")) {
                if (this->m_server_thread.joinable()) {
                    this->m_server_stop = true;
                    this->m_server_thread.join();
                    this->server.deinit();
                }
                this->m_server_stop = false;

                if (this->server.init(this->port)) {
                    std::cout << "[Host] Routing server listening on port " << this->port << '\n';
                    this->m_server_thread = std::thread([this]() {
                        while (!this->m_server_stop.load(std::memory_order_relaxed))
                            this->server.update();
                    });
                    if (!this->peer.connect(this->ip, this->port))
                        std::cout << "[Host] peer.connect() initiation failed\n";
                } else {
                    std::cout << "[Host] Failed to start server on port " << this->port << '\n';
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Connect")) {
                std::cout << "Connecting to ip " << Core::ip_into_octets(this->ip) << ":" <<this->port << std::endl;
                this->peer.connect(this->ip, this->port);
            }
            ImGui::End();

            Debug::DispatchDebugTextDrawing();
        }
    }
} // namespace Game
