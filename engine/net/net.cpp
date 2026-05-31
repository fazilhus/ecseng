#include "net.h"
#include "protocol.h"

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
                // Send PeerList to the newly connected peer
                {
                    flatbuffers::FlatBufferBuilder fbb;
                    std::vector<fb::PeerAddress> addrs;
                    addrs.reserve(m_peers.size());
                    for (const auto p : m_peers)
                        addrs.emplace_back(p->address.host, p->address.port);
                    auto peer_list = fb::CreatePeerListDirect(fbb, &addrs);
                    auto envelope = fb::CreateEnvelope(fbb, fb::Message_PeerList, peer_list.Union());
                    fbb.Finish(envelope);
                    ENetPacket* pkt = enet_packet_create(
                        fbb.GetBufferPointer(), fbb.GetSize(), ENET_PACKET_FLAG_RELIABLE);
                    if (enet_peer_send(e.peer, 0, pkt) != 0) enet_packet_destroy(pkt);
                }

                // Broadcast NewPeer to all existing peers
                {
                    flatbuffers::FlatBufferBuilder fbb;
                    fb::PeerAddress addr(e.peer->address.host, e.peer->address.port);
                    auto new_peer = fb::CreateNewPeer(fbb, &addr);
                    auto envelope = fb::CreateEnvelope(fbb, fb::Message_NewPeer, new_peer.Union());
                    fbb.Finish(envelope);
                    for (const auto p : m_peers) {
                        ENetPacket* pkt = enet_packet_create(
                            fbb.GetBufferPointer(), fbb.GetSize(), ENET_PACKET_FLAG_RELIABLE);
                        if (enet_peer_send(p, 0, pkt) != 0) enet_packet_destroy(pkt);
                    }
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
        while (enet_host_service(m_host, &e, 1000) > 0) {
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
                flatbuffers::Verifier verifier(
                    reinterpret_cast<const uint8_t*>(e.packet->data), e.packet->dataLength);
                if (!fb::VerifyEnvelopeBuffer(verifier)) {
                    std::cout << "[Peer] Received invalid packet (" << e.packet->dataLength << " bytes)\n";
                    enet_packet_destroy(e.packet);
                    break;
                }
                const fb::Envelope* envelope = fb::GetEnvelope(e.packet->data);
                switch (envelope->message_type()) {

                case fb::Message_PeerList: {
                    const fb::PeerList* peer_list = envelope->message_as_PeerList();
                    if (!peer_list || !peer_list->peers()) break;
                    for (const fb::PeerAddress* pa : *peer_list->peers()) {
                        ENetAddress addr;
                        addr.host = pa->host();
                        addr.port = pa->port();
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

                case fb::Message_NewPeer: {
                    const fb::NewPeer* new_peer = envelope->message_as_NewPeer();
                    if (!new_peer || !new_peer->address()) break;
                    const fb::PeerAddress* pa = new_peer->address();
                    ENetAddress addr;
                    addr.host = pa->host();
                    addr.port = pa->port();
                    char peer_ip[40];
                    enet_address_get_host_ip(&addr, peer_ip, 40);
                    ENetPeer* p = enet_host_connect(m_host, &addr, 2, 0);
                    ENetEvent temp_e;
                    if (enet_host_service(m_host, &temp_e, 1000) == 0 || temp_e.type != ENET_EVENT_TYPE_CONNECT) {
                        enet_peer_reset(p);
                        break;
                    }
                    m_peers.emplace_back(p);
                }
                break;

                case fb::Message_Ping: {
                    std::cout << "[Peer] Ping from " << ip << '\n';
                    flatbuffers::FlatBufferBuilder fbb;
                    auto pong = fb::CreatePong(fbb);
                    auto env_out = fb::CreateEnvelope(fbb, fb::Message_Pong, pong.Union());
                    fbb.Finish(env_out);
                    ENetPacket* pkt = enet_packet_create(
                        fbb.GetBufferPointer(), fbb.GetSize(), ENET_PACKET_FLAG_RELIABLE);
                    if (enet_peer_send(e.peer, 0, pkt) != 0) enet_packet_destroy(pkt);
                }
                break;

                case fb::Message_Pong:
                    std::cout << "[Peer] Pong from " << ip << '\n';
                break;

                default: break;
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
