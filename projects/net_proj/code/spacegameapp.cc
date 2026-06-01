//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include <array>
#include <random>
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
#include "gtx/quaternion.hpp"
#include <enet/enet.h>
#include "net/firewall.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32")
#endif

using namespace Display;
using namespace Render;

namespace Game {

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

namespace {
    constexpr float NET_TICK = 1.f / 20.f;

    inline fb::Vec3 to_fb(const glm::vec3& v) { return { v.x, v.y, v.z }; }
    inline fb::Quat to_fb(const glm::quat& q) { return { q.x, q.y, q.z, q.w }; }
    inline glm::vec3 from_fb(const fb::Vec3& v) { return { v.x(), v.y(), v.z() }; }
    inline glm::quat from_fb(const fb::Quat& q) { return { q.w(), q.x(), q.y(), q.z() }; }

#ifdef _WIN32
    std::string get_local_ip()
    {
        SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s == INVALID_SOCKET)
            return "0.0.0.0";
        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_port   = htons(53);
        inet_pton(AF_INET, "8.8.8.8", &dst.sin_addr);
        connect(s, (sockaddr*)&dst, sizeof(dst)); // no packet actually sent
        sockaddr_in local{};
        int len = sizeof(local);
        if (getsockname(s, (sockaddr*)&local, &len) != 0) {
            closesocket(s);
            return "0.0.0.0";
        }
        closesocket(s);
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));
        return buf;
    }
#else
    std::string get_local_ip() { return "0.0.0.0"; }
#endif

    void lan_scan_thread(std::atomic<uint32_t>* scan_result, std::atomic<bool>* scanning)
    {
#ifdef _WIN32
        // Bind to a fixed port to receive broadcast beacons
        SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s == INVALID_SOCKET) { *scanning = false; return; }
        BOOL yes = TRUE;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_port   = htons(6970);
        local.sin_addr.s_addr = INADDR_ANY;
        if (bind(s, (sockaddr*)&local, sizeof(local)) != 0) {
            closesocket(s);
            *scanning = false;
            return;
        }

        // Set non-blocking so we can poll with timeout
        u_long nb = 1;
        ioctlsocket(s, FIONBIO, &nb);

        auto t0 = std::chrono::steady_clock::now();
        char buf[64];
        sockaddr_in from{};
        int fl = sizeof(from);

        while (std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count() < 2.0f) {
            int n = recvfrom(s, buf, sizeof(buf), 0, (sockaddr*)&from, &fl);
            if (n == 4 && std::memcmp(buf, "ECS1", 4) == 0) {
                // Found a routing server beacon — store the source IP
                uint32_t expected = 0;
                if (scan_result->compare_exchange_strong(expected, from.sin_addr.s_addr)) {
                    char buf2[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &from.sin_addr, buf2, sizeof(buf2));
                    std::cout << "[Scan] Found routing server at "
                              << buf2 << ":6969\n";
                }
                break;
            }
            fl = sizeof(from); // reset addr length
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        closesocket(s);
#endif
        *scanning = false;
    }
} // anonymous

//------------------------------------------------------------------------------
/**
*/
SpaceGameApp::SpaceGameApp()
    : window(nullptr), world(nullptr),
      ip(Core::octets_into_ip({127, 0, 0, 1})), port(6969)
{
    Core::ensure_udp_firewall_rule("ECS SpaceGame");

    std::mt19937 rng{ std::random_device{}() };
    m_local_player_id = std::uniform_int_distribution(1u, UINT32_MAX)(rng);
    std::cout << "[Net] Local player_id = " << m_local_player_id << '\n';

    this->peer.init();

    // Detect local LAN IP for display in the Host panel
    m_local_ip_str = get_local_ip();
    std::cout << "[Net] Local LAN IP: " << m_local_ip_str << '\n';
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
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        RenderDevice::Init();
        this->window->SetUiRender([this]() { this->RenderUI(); });
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void SpaceGameApp::Broadcast(const flatbuffers::FlatBufferBuilder& fbb, bool reliable) {
    const enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    for (auto* ep : peer.m_peers) {
        ENetPacket* pkt = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
        if (enet_peer_send(ep, 0, pkt) != 0)
            enet_packet_destroy(pkt);
    }
}

//------------------------------------------------------------------------------
void SpaceGameApp::ProcessNetEvents() {
    for (ENetPeer* ep : peer.m_connected_peers) {
        {
            flatbuffers::FlatBufferBuilder fbb;
            auto name = fbb.CreateString("");
            auto join = fb::CreatePlayerJoin(fbb, m_local_player_id, name);
            auto env  = fb::CreateEnvelope(fbb, fb::Message_PlayerJoin, join.Union());
            fbb.Finish(env);
            ENetPacket* pkt = enet_packet_create(
                fbb.GetBufferPointer(), fbb.GetSize(), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(ep, 0, pkt);
        }

        const auto ghost = world->CreateEntity();
        world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(
            ghost,
            glm::vec3(0.f),
            glm::identity<glm::quat>(),
            glm::vec3(1.f));
        world->AddComponent<Ecs::ModelComponent, Ecs::CT_MODEL>(
            ghost, LoadModel("assets/space/spaceship.glb"));
        world->AddComponent<Ecs::DeadReckoningComponent, Ecs::CT_DEAD_RECKONING>(
            ghost, 0u);

        RemotePeer rp;
        rp.enet_peer  = ep;
        rp.player_id  = 0;
        rp.ghost_ship = ghost;
        m_remote_peers[ep] = rp;

        char addr[40];
        enet_address_get_host_ip(&ep->address, addr, 40);
        std::cout << "[Game] Spawned ghost ship for peer at " << addr << '\n';
    }
    peer.m_connected_peers.clear();

    for (ENetPeer* ep : peer.m_disconnected_peers) {
        if (auto it = m_remote_peers.find(ep); it != m_remote_peers.end()) {
            world->DestroyEntity(it->second.ghost_ship);
            m_remote_peers.erase(it);
            std::cout << "[Game] Removed ghost ship for disconnected peer\n";
        }
    }
    peer.m_disconnected_peers.clear();

    for (const auto& [from, data] : peer.m_inbox) {
        switch (const fb::Envelope* env = fb::GetEnvelope(data.data()); env->message_type()) {

        case fb::Message_PlayerJoin: {
            const fb::PlayerJoin* pj = env->message_as_PlayerJoin();
            if (!pj) break;
            if (auto it = m_remote_peers.find(from); it != m_remote_peers.end()) {
                it->second.player_id = pj->player_id();
                auto& dr = world->GetComponent<Ecs::DeadReckoningComponent>(
                    it->second.ghost_ship);
                dr.net_entity_id = pj->player_id();
                std::cout << "[Game] PlayerJoin: remote player_id = " << pj->player_id() << '\n';
            }
        }
        break;

        case fb::Message_PlayerLeft: {
            if (auto it = m_remote_peers.find(from); it != m_remote_peers.end()) {
                world->DestroyEntity(it->second.ghost_ship);
                m_remote_peers.erase(it);
                std::cout << "[Game] PlayerLeft: removed ghost ship\n";
            }
        }
        break;

        case fb::Message_EntityState: {
            const fb::EntityState* es = env->message_as_EntityState();
            if (!es) break;
            auto it = m_remote_peers.find(from);
            if (it == m_remote_peers.end()) break;

            if (es->entity_id() == it->second.player_id && it->second.player_id != 0) {
                if (!es->pos() || !es->rot() || !es->vel()) break;
                auto& dr = world->GetComponent<Ecs::DeadReckoningComponent>(
                    it->second.ghost_ship);
                dr.last_pos  = from_fb(*es->pos());
                dr.last_rot  = from_fb(*es->rot());
                dr.last_vel  = from_fb(*es->vel());
                dr.recv_time = m_game_time;
            } else {
                if (!es->pos() || !es->dir()) break;
                const glm::vec3 spawn_pos = from_fb(*es->pos());
                const glm::vec3 dir       = from_fb(*es->dir());
                const float     speed     = es->vel()
                    ? glm::length(from_fb(*es->vel()))
                    : 30.f;

                const auto p = world->CreateEntity();
                const auto rot = glm::quatLookAt(
                    glm::normalize(dir), glm::vec3(0.f, 1.f, 0.f));
                world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(
                    p, spawn_pos, rot, glm::vec3(1.f));
                world->AddComponent<Ecs::ModelComponent, Ecs::CT_MODEL>(
                    p, m_laser_model);
                world->AddComponent<Ecs::ColliderComponent, Ecs::CT_COLLIDER>(
                    p, m_laser_cmesh, true);
                world->AddComponent<Ecs::ProjectileComponent, Ecs::CT_PROJECTILE>(
                    p, glm::normalize(dir), speed);
            }
        }
        break;

        case fb::Message_PlayerDeath: {
        }
        break;

        case fb::Message_PlayerRespawn: {
            const fb::PlayerRespawn* pr = env->message_as_PlayerRespawn();
            if (!pr || !pr->pos()) break;
            if (auto it = m_remote_peers.find(from); it != m_remote_peers.end()) {
                auto& dr = world->GetComponent<Ecs::DeadReckoningComponent>(
                    it->second.ghost_ship);
                dr.last_pos  = from_fb(*pr->pos());
                dr.last_vel  = glm::vec3(0.f);
                dr.recv_time = m_game_time;
            }
        }
        break;

        default:
            break;
        }
    }
    peer.m_inbox.clear();
}

//------------------------------------------------------------------------------
/**
*/
void SpaceGameApp::Run() {
    int w, h;
    this->window->GetSize(w, h);

    this->world = new Ecs::World{};

    glm::mat4 projection = glm::perspective(
        glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
    Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
    cam->projection = projection;

    // ── Load assets ──
    ModelId models[6] = {
        LoadModel("assets/space/Asteroid_1.glb"),
        LoadModel("assets/space/Asteroid_2.glb"),
        LoadModel("assets/space/Asteroid_3.glb"),
        LoadModel("assets/space/Asteroid_4.glb"),
        LoadModel("assets/space/Asteroid_5.glb"),
        LoadModel("assets/space/Asteroid_6.glb")
    };

    Physics::ColliderMeshId colliderMeshes[6] = {
        Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
    };

    m_laser_model = LoadModel("assets/space/laser.glb");
    m_laser_cmesh = Physics::LoadColliderMesh("assets/space/laser.glb");

    // ── Asteroids (near) ──
    std::vector<Ecs::EntityID> asteroids;
    for (auto i = 0; i < 100; i++) {
        const auto ri  = static_cast<size_t>(Core::FastRandom() % 6);
        constexpr auto span = 30.0f;
        const auto pos  = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span);
        const auto axis = glm::normalize(pos);
        const auto rot  = glm::quat(Core::RandomFloatNTP(), axis);
        const auto e    = world->CreateEntity();
        asteroids.push_back(e);
        world->AddComponent<Ecs::ModelComponent,   Ecs::CT_MODEL>   (e, models[ri]);
        world->AddComponent<Ecs::ColliderComponent, Ecs::CT_COLLIDER>(e, colliderMeshes[ri], true);
        world->AddComponent<Ecs::TransformComponent,Ecs::CT_TRANSFORM>(e, pos, rot, glm::vec3(1.f));
    }

    // ── Asteroids (far) ──
    for (int i = 0; i < 50; i++) {
        const auto ri   = static_cast<size_t>(Core::FastRandom() % 6);
        constexpr auto span = 100.0f;
        const auto pos  = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span);
        const auto axis = glm::normalize(pos);
        const auto rot  = glm::quat(Core::RandomFloatNTP(), axis);
        const auto e    = world->CreateEntity();
        asteroids.push_back(e);
        world->AddComponent<Ecs::ModelComponent,   Ecs::CT_MODEL>   (e, models[ri]);
        world->AddComponent<Ecs::ColliderComponent, Ecs::CT_COLLIDER>(e, colliderMeshes[ri], true);
        world->AddComponent<Ecs::TransformComponent,Ecs::CT_TRANSFORM>(e, pos, rot, glm::vec3(1.f));
    }

    // ── Waypoints ──
    std::vector<Ecs::EntityID> waypoints;
    for (auto i = 0; i < 4; ++i)
        waypoints.push_back(world->CreateEntity());
    {
        const auto zero_rot = glm::quat_cast(glm::identity<glm::mat4>());
        world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[0], glm::vec3(-10.f,0.f,-10.f), zero_rot, glm::vec3(1.f));
        world->AddComponent<Ecs::WaypointComponent,  Ecs::CT_WAYPOINT> (waypoints[0], waypoints[3], waypoints[1]);
        world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[1], glm::vec3(-10.f,0.f, 10.f), zero_rot, glm::vec3(1.f));
        world->AddComponent<Ecs::WaypointComponent,  Ecs::CT_WAYPOINT> (waypoints[1], waypoints[0], waypoints[2]);
        world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[2], glm::vec3( 10.f,0.f, 10.f), zero_rot, glm::vec3(1.f));
        world->AddComponent<Ecs::WaypointComponent,  Ecs::CT_WAYPOINT> (waypoints[2], waypoints[1], waypoints[3]);
        world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(waypoints[3], glm::vec3( 10.f,0.f,-10.f), zero_rot, glm::vec3(1.f));
        world->AddComponent<Ecs::WaypointComponent,  Ecs::CT_WAYPOINT> (waypoints[3], waypoints[2], waypoints[0]);
    }

    // ── Skybox ──
    std::vector<const char*> skybox(6, "assets/space/bg.png");
    TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
    RenderDevice::SetSkybox(skyboxId);

    Input::Keyboard* kbd = Input::GetDefaultKeyboard();

    // ── Lights ──
    constexpr auto numLights = 40;
    for (auto i = 0; i < numLights; i++) {
        Render::PointLightId lights[numLights];
        const glm::vec3 pos   = glm::vec3(Core::RandomFloatNTP(),Core::RandomFloatNTP(),Core::RandomFloatNTP()) * 20.f;
        const auto color = glm::vec3(Core::RandomFloat(), Core::RandomFloat(), Core::RandomFloat());
        lights[i] = Render::LightServer::CreatePointLight(
            pos, color, Core::RandomFloat() * 4.f, 1.f + 15.f + Core::RandomFloat() * 10.f);
    }

    // ── Local player ship ──
    const auto ship_model = LoadModel("assets/space/spaceship.glb");
    const std::vector<glm::vec3> ship_collider = {
        {1.40173f,0.f,-0.225342f}, {1.33578f,0.f,0.088893f},
        {0.227107f,-0.200232f,-0.588618f}, {0.227107f,0.228809f,-0.588618f},
        {0.391073f,-0.130853f,1.28339f},  {0.134787f,0.f,1.68965f},
        {0.134787f,0.250728f,0.647422f},
        {-1.40173f,0.f,-0.225342f}, {-1.33578f,0.f,0.088893f},
        {-0.227107f,-0.200232f,-0.588618f},{-0.227107f,0.228809f,-0.588618f},
        {-0.391073f,-0.130853f,1.28339f}, {-0.134787f,0.f,1.68965f},
        {-0.134787f,0.250728f,0.647422f},
        {0.f,0.525049f,-0.392836f},{0.f,0.739624f,0.102582f},{0.f,-0.244758f,0.284825f}
    };

    m_ship = world->CreateEntity();
    world->AddComponent<Ecs::TransformComponent, Ecs::CT_TRANSFORM>(
        m_ship, glm::vec3(0.f),
        glm::quat(glm::radians(glm::vec3(0.f,90.f,0.f))), glm::vec3(1.f));
    world->AddComponent<Ecs::ModelComponent,    Ecs::CT_MODEL>   (m_ship, ship_model);
    world->AddComponent<Ecs::CameraComponent,   Ecs::CT_CAMERA>  (m_ship,
        glm::mat4(1.f),
        glm::perspective(glm::radians(90.f), static_cast<float>(w)/static_cast<float>(h), 0.01f, 1000.f));
    world->AddComponent<Ecs::MovementComponent, Ecs::CT_MOVEMENT>(m_ship);
    world->AddComponent<Ecs::PlayerCharacterComponent, Ecs::CT_PLAYER_CHARACTER>(m_ship);
    world->AddComponent<Ecs::CollisionComponent,       Ecs::CT_COLLISION>(m_ship, ship_collider);
    world->AddComponent<Ecs::ParticleEmitterComponent, Ecs::CT_PARTICLE_EMITTER>(
        m_ship, glm::vec3(0.f,0.f,-0.5f), glm::vec4(0.38f,0.76f,0.95f,1.f));
    world->AddComponent<Ecs::ProjectileSpawnerComponent, Ecs::CT_PROJECTILE_SPAWNER>(
        m_ship, glm::vec3(0.f,0.f,2.f), 30.f, m_laser_model, m_laser_cmesh);

    world->Start();

    auto dt = 0.01667f;

    while (this->window->IsOpen()) {
        auto timeStart = std::chrono::steady_clock::now();

        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        this->window->Update();

        peer.update();
        m_game_time += dt;
        ProcessNetEvents();

        world->BeforeFrame();

        if (kbd->pressed[Input::Key::Code::End])
            ShaderResource::ReloadShaders();

        for (auto e : asteroids) {
            auto& tc = world->GetComponent<Ecs::TransformComponent>(e);
            const auto axis  = glm::axis(tc.rot);
            float angle = glm::angle(tc.rot);
            angle += 0.05f * dt;
            tc.rot = glm::rotate(angle, axis);
            tc.transform = glm::translate(tc.pos) * glm::mat4_cast(tc.rot);
        }

        const glm::vec3 pos_before = world->GetComponent<Ecs::TransformComponent>(m_ship).pos;
        world->PhysicsUpdate(dt);
        const glm::vec3 pos_after  = world->GetComponent<Ecs::TransformComponent>(m_ship).pos;

        if (!peer.m_peers.empty() && glm::distance(pos_before, pos_after) > 1.f) {
            {
                flatbuffers::FlatBufferBuilder fbb;
                auto death = fb::CreatePlayerDeath(fbb, m_local_player_id);
                auto env   = fb::CreateEnvelope(fbb, fb::Message_PlayerDeath, death.Union());
                fbb.Finish(env);
                Broadcast(fbb, true);
            }
            {
                flatbuffers::FlatBufferBuilder fbb;
                const fb::Vec3 pos = to_fb(pos_after);
                auto respawn = fb::CreatePlayerRespawn(fbb, m_local_player_id, &pos);
                auto env     = fb::CreateEnvelope(fbb, fb::Message_PlayerRespawn, respawn.Union());
                fbb.Finish(env);
                Broadcast(fbb, true);
            }
        }

        world->Update(dt);

        if (kbd->pressed[Input::Key::Code::Space] && !peer.m_peers.empty()) {
            const auto& t  = world->GetComponent<Ecs::TransformComponent>(m_ship);
            const auto& ps = world->GetComponent<Ecs::ProjectileSpawnerComponent>(m_ship);
            const glm::vec3 spawn_pos = glm::vec3(t.transform * glm::vec4(ps.offset, 1.f));
            const glm::vec3 dir       = glm::normalize(
                glm::vec3(t.transform * glm::vec4(0.f, 0.f, 1.f, 0.f)));
            const glm::vec3 vel       = dir * ps.speed;

            flatbuffers::FlatBufferBuilder fbb;
            const fb::Vec3 p  = to_fb(spawn_pos);
            const fb::Vec3 d  = to_fb(dir);
            const fb::Vec3 v  = to_fb(vel);
            const fb::Quat r  = {};
            const uint32_t id = (m_local_player_id << 8) | m_proj_counter++;
            auto es  = fb::CreateEntityState(fbb, id, &p, &r, &v, &d, 0);
            auto env = fb::CreateEnvelope(fbb, fb::Message_EntityState, es.Union());
            fbb.Finish(env);
            Broadcast(fbb, true);
        }

        m_net_tick_accum += dt;
        if (m_net_tick_accum >= NET_TICK && !peer.m_peers.empty()) {
            m_net_tick_accum -= NET_TICK;

            const auto& t = world->GetComponent<Ecs::TransformComponent>(m_ship);
            const auto& m = world->GetComponent<Ecs::MovementComponent>(m_ship);

            flatbuffers::FlatBufferBuilder fbb;
            const fb::Vec3 pos = to_fb(t.pos);
            const fb::Quat rot = to_fb(t.rot);
            const fb::Vec3 vel = to_fb(m.linearVelocity);
            const glm::vec3 fwd = t.transform * glm::vec4(0.f, 0.f, 1.f, 0.f);
            const fb::Vec3 dir  = to_fb(fwd);
            auto es  = fb::CreateEntityState(fbb, m_local_player_id, &pos, &rot, &vel, &dir, 0);
            auto env = fb::CreateEnvelope(fbb, fb::Message_EntityState, es.Union());
            fbb.Finish(env);
            Broadcast(fbb, false);
        }

        world->BeforeDraw();
        world->Draw();

        for (auto i = 0; i < 4; ++i) {
            const auto& tc = world->GetComponent<Ecs::TransformComponent>(waypoints[i]);
            Debug::DrawBox(tc.pos, tc.rot, 0.25f,
                glm::vec4(1.f - 0.33f*i, 0.f, 0.33f*i, 1.f));
        }

        RenderDevice::Render(this->window, dt);
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
void SpaceGameApp::Exit() {
    this->window->Close();
    if (this->server.m_initialized) {
        this->server.deinit();
    }
}

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
        ImGui::Text("Network  (player_id: %u)", m_local_player_id);
        ImGui::Text("My LAN IP: %s  (share with other players)", m_local_ip_str.c_str());
        ImGui::Text("P2P peers connected: %d", (int)peer.m_peers.size());

        std::array<int, 4> octets = Core::ip_into_octets(this->ip);
        if (ImGui::InputInt4("IP Address", &octets[0]))
            this->ip = Core::octets_into_ip(octets);
        ImGui::SameLine();
        int p = this->port;
        if (ImGui::InputInt("Port", &p))
            this->port = static_cast<uint16_t>(p & 0xFFFF);

        if (!this->server.m_live && !this->peer.is_live()) {
            if (ImGui::Button("Host")) {
                // if (this->m_server_thread.joinable()) {
                //     this->m_server_stop = true;
                //     this->server.m_live = true;
                //     this->m_server_thread.join();
                //     this->server.deinit();
                // }
                // this->m_server_stop = false;

                if (this->server.init(this->port)) {
                    std::cout << "[Host] Routing server listening on port " << this->port << '\n';
                    this->m_server_thread = std::thread([this]() {
                        this->server.m_live = true;
                        while (!this->m_server_stop.load(std::memory_order_relaxed))
                            this->server.update();
                    });
                    if (!this->peer.connect(Core::octets_into_ip({127, 0, 0, 1}), this->port))
                        std::cout << "[Host] peer.connect() initiation failed\n";
                } else {
                    std::cout << "[Host] Failed to start server on port " << this->port << '\n';
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Connect")) {
                std::cout << "[Peer] Connecting to "
                          << Core::ip_into_octets(this->ip) << ':' << this->port << '\n';
                this->peer.connect(this->ip, this->port);
            }
        } else {
            if (ImGui::Button("Disconnect")) {
                std::cout << "[Peer] Disconnecting\n";
                if (this->m_server_thread.joinable()) {
                    this->m_server_stop = true;
                    this->server.m_live = false;
                    this->m_server_thread.join();
                    // this->server.deinit();
                }
                this->m_server_stop = false;
                for (auto [fst, snd] : m_remote_peers) {
                    world->DestroyEntity(snd.ghost_ship);
                    m_remote_peers.erase(fst);
                    std::cout << "[Game] PlayerLeft: removed ghost ship\n";
                }
                this->peer.disconnect();
            }
        }

        // if (!m_scanning.load() && ImGui::Button("Scan LAN")) {
        //     m_scanning = true;
        //     std::cout << "[Scan] Scanning for routing servers on LAN...\n";
        //     std::thread(lan_scan_thread, &m_scan_ip, &m_scanning).detach();
        // }
        // if (m_scanning.load()) {
        //     ImGui::SameLine();
        //     ImGui::TextColored(ImVec4(1,1,0,1), "Scanning...");
        // }

        ImGui::End();
        Debug::DispatchDebugTextDrawing();
    }
}

} // namespace Game
