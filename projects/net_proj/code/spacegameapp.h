#pragma once
//------------------------------------------------------------------------------
/**
    Space game application — net_proj

    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/app.h"
#include "net/net.h"
#include "core/world.h"
#include "render/window.h"
#include "render/model.h"
#include "render/physics.h"
#include <atomic>
#include <thread>
#include <unordered_map>

namespace Game {

    struct RemotePeer {
        ENetPeer*     enet_peer  = nullptr;
        uint32_t      player_id  = 0;
        Ecs::EntityID ghost_ship = 0;
    };

    class SpaceGameApp : public Core::App {
    public:
        SpaceGameApp();
        virtual ~SpaceGameApp() override;

        virtual bool Open()  override;
        virtual void Run()   override;
        virtual void Exit()  override;

    private:
        void RenderUI();

        void ProcessNetEvents();
        void Broadcast(const flatbuffers::FlatBufferBuilder& fbb, bool reliable);

        Display::Window* window = nullptr;
        Ecs::World*      world  = nullptr;

        Core::peer   peer;
        Core::server server;
        std::thread         m_server_thread;
        std::atomic<bool>   m_server_stop{false};
        uint32_t ip;
        uint16_t port;

        uint32_t m_local_player_id = 0;
        float m_game_time = 0.f;
        float m_net_tick_accum = 0.f;
        uint8_t m_proj_counter = 0;
        std::unordered_map<ENetPeer*, RemotePeer> m_remote_peers;
        Ecs::EntityID m_ship = 0;
        Render::ModelId         m_laser_model{};
        Physics::ColliderMeshId m_laser_cmesh{};
    };

} // namespace Game
