#include "net.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <ranges>


namespace Core {
    server::server()
        : m_initialized(false), m_port(6969), m_host(nullptr), m_peers({}) {}

    server::~server() { deinit(); }

    bool server::init(const uint16_t port) {
        if (m_initialized) return true;
        if (enet_initialize() != 0) {
            std::cout << "[Routing Server] failed to init enet\n";
            return false;
        }
        m_port = port;

        ENetAddress addr;
        addr.host = ENET_HOST_ANY;
        addr.port = m_port;
        m_host = enet_host_create(&addr, 32, 2, 0, 0);
        if (m_host == nullptr) {
            std::cout << "[Routing Server] failed to create enet host\n";
            return false;
        }

        m_initialized = true;
        return true;
    }

    void server::deinit() {
        if (!m_initialized)
            return;
        for (const auto peer: m_peers) {
            std::cout << "[Routing Server] Disconnected from " << Core::ip_into_octets(peer->address.host) << '\n';
            enet_peer_reset(peer);
        }
        if (m_host != nullptr) {
            std::cout << "[Routing Server] Deinitialized\n";
            enet_host_destroy(m_host);
        }

        enet_deinitialize();
        m_initialized = false;
    }

    void server::update() {
        if (!m_initialized)
            return;
        ENetEvent e;
        while (enet_host_service(m_host, &e, 32) > 0) {
            char ip[40];
            enet_address_get_host_ip(&e.peer->address, ip, 40);

            switch (e.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                m_buf.reset();
                m_buf.write(peer_cmd::PeerList);
                m_buf.write(static_cast<uint32_t>(m_peers.size()));
                for (auto p: m_peers) {
                    m_buf.write(p->address.host);
                    m_buf.write(p->address.port);
                }
                ENetPacket* packet_peer_list = enet_packet_create(
                    m_buf.m_buffer, m_buf.m_size, ENET_PACKET_FLAG_RELIABLE
                );
                if (enet_peer_send(e.peer, 0, packet_peer_list) != 0) { enet_packet_destroy(packet_peer_list); }

                m_buf.reset();
                m_buf.write(peer_cmd::NewPeer);
                m_buf.write(e.peer->address.host);
                m_buf.write(e.peer->address.port);
                for (const auto peer: m_peers) {
                    if (ENetPacket* pkt = enet_packet_create(m_buf.m_buffer, m_buf.m_size, ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(peer, 0, pkt) != 0) { enet_packet_destroy(pkt); }
                }

                m_peers.emplace_back(e.peer);
                enet_host_flush(m_host);
            }
            break;
            case ENET_EVENT_TYPE_DISCONNECT: { m_peers.erase(std::ranges::find(m_peers, e.peer)); }
            break;
            case ENET_EVENT_TYPE_RECEIVE: {
                std::cout << "[Routing Server] Data received lol! " << e.packet->dataLength << " bytes from " << e.peer
                    << '\n';
            }
            break;
            default:
                break;
            }
        }
    }

    peer::peer()
        : m_initialized(false), m_host(nullptr), m_server_peer(nullptr), m_peers({}) {}

    peer::~peer() { deinit(); }

    bool peer::init() {
        if (m_initialized) return true;
        if (enet_initialize() != 0)
            return false;

        ENetAddress host_addr;
        host_addr.host = ENET_HOST_ANY;
        host_addr.port = ENET_PORT_ANY;
        m_host = enet_host_create(&host_addr, 32, 2, 0, 0);
        if (m_host == nullptr)
            return false;

        m_initialized = true;
        return true;
    }

    void peer::deinit() {
        if (!m_initialized)
            return;
        // for (const auto peer: m_peers) { enet_peer_reset(peer); }
        // if (m_server_peer != nullptr)
        //     enet_peer_reset(m_server_peer);
        if (m_host != nullptr)
            enet_host_destroy(m_host);

        enet_deinitialize();
        m_initialized = false;
    }

    bool peer::connect(const uint32_t ip, const uint16_t port) {
        ENetAddress server_addr;
        server_addr.host = ip;
        server_addr.port = port;
        m_server_peer = enet_host_connect(m_host, &server_addr, 2, 0);
        if (m_server_peer == nullptr) {
            std::cout << "[Peer] failed to connect to routing server at " << ip << ":" << port << '\n';
            return false;
        }

        ENetEvent e;
        while (enet_host_service(m_host, &e, 32) > 0) {
            std::cout << "[Peer] received event " << e.type << " from " << ip << ":" << port << '\n';
            if (e.type == ENET_EVENT_TYPE_CONNECT) {
                return true; // Connection established
            }
            if (e.type == ENET_EVENT_TYPE_DISCONNECT || e.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(e.packet);
            }
        }
        enet_peer_reset(m_server_peer);
        std::cout << "[Peer] failed to receive back from routing server\n";
        return false;
    }

    void peer::disconnect()  {
        if (m_server_peer != nullptr) {
            enet_peer_disconnect_now(m_server_peer, 0);
        }
        for (const auto p: m_peers) {
            enet_peer_disconnect_now(p, 0);
        }
        m_peers.clear();
        m_server_peer = nullptr;
    }

    void peer::update() {
        if (!m_initialized)
            return;
        ENetEvent e;
        while (enet_host_service(m_host, &e, 0) > 0) {
            char ip[40];
            enet_address_get_host_ip(&e.peer->address, ip, 40);

            switch (e.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::cout << "[Peer] Connected to " << ip << ":" << &e.peer->address << '\n';
            }
            break;
            case ENET_EVENT_TYPE_DISCONNECT: {
                std::cout << "[Peer] Disconnected from " << ip << ":" << &e.peer->address << '\n';
                if (auto it = std::ranges::find(m_peers, e.peer);
                    it != m_peers.end()) { m_peers.erase(it); }
            }
            break;
            case ENET_EVENT_TYPE_RECEIVE: {
                netrdbuf read_buffer(reinterpret_cast<char*>(e.packet->data), e.packet->dataLength);
                peer_cmd cmd;
                if (!read_buffer.read<peer_cmd>(cmd)) break;
                switch (cmd) {
                case peer_cmd::PeerList: {
                    uint32_t num_peers;
                    read_buffer.read<uint32_t>(num_peers);
                    for (uint32_t i = 0; i < num_peers; i++) {
                        ENetAddress addr;
                        read_buffer.read<uint32_t>(addr.host);
                        read_buffer.read<uint16_t>(addr.port);

                        char peer_ip[40];
                        enet_address_get_host_ip(&addr, peer_ip, 40);
                        ENetPeer* p = enet_host_connect(m_host, &addr, 2, 0);
                        ENetEvent temp_e;
                        if (enet_host_service(m_host, &temp_e, 1000) == 0 || temp_e.type != ENET_EVENT_TYPE_CONNECT) {
                            enet_peer_reset(p);
                            continue;
                        }
                        m_peers.emplace_back(p);
                    }
                }
                break;
                case peer_cmd::NewPeer: {
                    ENetAddress addr;
                    read_buffer.read<uint32_t>(addr.host);
                    read_buffer.read<uint16_t>(addr.port);

                    char peer_ip[40];
                    enet_address_get_host_ip(&addr, peer_ip, 40);
                    ENetPeer* p = enet_host_connect(m_host, &addr, 2, 0);
                    ENetEvent temp_e;
                    if (enet_host_service(m_host, &temp_e, 1000) == 0 || temp_e.type != ENET_EVENT_TYPE_CONNECT) {
                        enet_peer_reset(p);
                        continue;
                    }
                    m_peers.emplace_back(p);
                }
                break;
                case peer_cmd::Ping: {
                    std::cout << "Ping from " << ip << ":" << &e.peer->address << '\n';

                    m_write_buf.reset();
                    m_write_buf.write(peer_cmd::Pong);
                    ENetPacket* pack = enet_packet_create(
                        m_write_buf.m_buffer, m_write_buf.m_size, ENET_PACKET_FLAG_RELIABLE
                    );
                    enet_peer_send(e.peer, 0, pack);
                }
                break;
                case peer_cmd::Pong: { std::cout << "Pong from " << ip << ":" << &e.peer->address << '\n'; }
                break;
                case peer_cmd::None:
                default: {}
                    break;
                }
                enet_packet_destroy(e.packet);
            }
            break;
            default:
                break;
            }
        }
    }

    std::array<int, 4> ip_into_octets(const uint32_t ip) {
        return {
            static_cast<int>(ip >> 24 & 0xFF),
            static_cast<int>(ip >> 16 & 0xFF),
            static_cast<int>(ip >> 8 & 0xFF),
            static_cast<int>(ip & 0xFF)
        };
    }

    uint32_t octets_into_ip(const std::array<int, 4> octets) {
        return (static_cast<uint32_t>(octets[0]) << 24)
            | (static_cast<uint32_t>(octets[1]) << 16)
            | (static_cast<uint32_t>(octets[2]) << 8)
            | static_cast<uint32_t>(octets[3]);
    }
} // namespace core
