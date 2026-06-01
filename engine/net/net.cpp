#include "net.h"
#include "protocol.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <ranges>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32")
#endif
#include <chrono>
#include <thread>


namespace Net {

#ifdef _WIN32
        std::string get_local_ip() {
            SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
            if (s == INVALID_SOCKET)
                return "0.0.0.0";
            sockaddr_in dst{};
            dst.sin_family = AF_INET;
            dst.sin_port = htons(53);
            inet_pton(AF_INET, "8.8.8.8", &dst.sin_addr);
            connect(s, reinterpret_cast<sockaddr*>(&dst), sizeof(dst)); // no packet actually sent
            sockaddr_in local{};
            int len = sizeof(local);
            if (getsockname(s, reinterpret_cast<sockaddr*>(&local), &len) != 0) {
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

        void lan_scan_thread(std::atomic<uint32_t>* scan_result, std::atomic<bool>* scanning) {
#ifdef _WIN32
            // Bind to a fixed port to receive broadcast beacons
            SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
            if (s == INVALID_SOCKET) {
                *scanning = false;
                return;
            }
            auto yes = TRUE;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(yes));

            sockaddr_in local{};
            local.sin_family = AF_INET;
            local.sin_port = htons(6970);
            local.sin_addr.s_addr = INADDR_ANY;
            if (bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0) {
                closesocket(s);
                *scanning = false;
                return;
            }

            // Set non-blocking so we can poll with timeout
            u_long nb = 1;
            ioctlsocket(s, FIONBIO, &nb);

            const auto t0 = std::chrono::steady_clock::now();
            char buf[64];
            sockaddr_in from{};
            int fl = sizeof(from);

            while (std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count() < 2.0f) {
                if (const int n = recvfrom(s, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fl);
                    n == 4 && std::memcmp(buf, "ECS1", 4) == 0) {
                    // Found a routing server beacon — store the source IP
                    if (uint32_t expected = 0;
                        scan_result->compare_exchange_strong(expected, from.sin_addr.s_addr)) {
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

    server::server()
        : m_initialized(false), m_live(false), m_port(6969), m_host(nullptr), m_peers({}) {}

    server::~server() { deinit(); }

    bool server::init(const uint16_t port) {
        if (m_initialized)
            return true;
        // if (enet_initialize() != 0) {
        //     std::cout << "[Routing Server] failed to init enet\n";
        //     return false;
        // }
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
            char ip[40];
            enet_address_get_host_ip(&peer->address, ip, 40);
            std::cout << "[Routing Server] Disconnected from " << ip << '\n';
            enet_peer_reset(peer);
        }
        if (m_host != nullptr) {
            std::cout << "[Routing Server] Deinitialized\n";
            enet_host_destroy(m_host);
        }
        // enet_deinitialize();
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
                    for (const auto p: m_peers)
                        addrs.emplace_back(p->address.host, p->address.port);
                    auto peer_list = fb::CreatePeerListDirect(fbb, &addrs);
                    auto envelope = fb::CreateEnvelope(fbb, fb::Message_PeerList, peer_list.Union());
                    fbb.Finish(envelope);
                    ENetPacket* pkt = enet_packet_create(
                        fbb.GetBufferPointer(), fbb.GetSize(), ENET_PACKET_FLAG_RELIABLE
                    );
                    if (enet_peer_send(e.peer, 0, pkt) != 0)
                        enet_packet_destroy(pkt);
                }
                // Broadcast NewPeer to all existing peers
                {
                    flatbuffers::FlatBufferBuilder fbb;
                    fb::PeerAddress addr(e.peer->address.host, e.peer->address.port);
                    auto new_peer = fb::CreateNewPeer(fbb, &addr);
                    auto envelope = fb::CreateEnvelope(fbb, fb::Message_NewPeer, new_peer.Union());
                    fbb.Finish(envelope);
                    for (const auto p: m_peers) {
                        ENetPacket* pkt = enet_packet_create(
                            fbb.GetBufferPointer(), fbb.GetSize(), ENET_PACKET_FLAG_RELIABLE
                        );
                        if (enet_peer_send(p, 0, pkt) != 0)
                            enet_packet_destroy(pkt);
                    }
                }
                m_peers.emplace_back(e.peer);
                enet_host_flush(m_host);
                std::cout << "[Routing Server] Peer connected from " << ip << '\n';
            }
            break;
            case ENET_EVENT_TYPE_DISCONNECT: {
                m_peers.erase(std::ranges::find(m_peers, e.peer));
                std::cout << "[Routing Server] Peer disconnected: " << ip << '\n';
            }
            break;
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(e.packet);
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
        if (m_initialized)
            return true;
        // if (enet_initialize() != 0) {
        //     return false;
        // }

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
        if (m_host != nullptr)
            enet_host_destroy(m_host);
        // enet_deinitialize();
        m_initialized = false;
    }

    bool peer::connect(const uint32_t ip, const uint16_t port) {
        if (!m_initialized) {
            std::cerr << "[Peer] connect() called before init()\n";
            return false;
        }
        ENetAddress server_addr;
        server_addr.host = ip;
        server_addr.port = port;

        char addr_str[40]{};
        enet_address_get_host_ip(&server_addr, addr_str, sizeof(addr_str));
        std::cout << "[Peer] Connecting to routing server at " << addr_str << ':' << port << '\n';

        m_server_peer = enet_host_connect(m_host, &server_addr, 2, 0);
        if (m_server_peer == nullptr) {
            std::cerr << "[Peer] enet_host_connect returned nullptr (no peer slots?)\n";
            return false;
        }
        // Non-blocking: flush the SYN; CONNECT arrives in next update().
        enet_host_flush(m_host);
        return true;
    }

    void peer::disconnect() {
        if (m_server_peer != nullptr)
            enet_peer_disconnect(m_server_peer, 0);
        for (const auto p: m_peers)
            enet_peer_disconnect(p, 0);
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
                if (e.peer == m_server_peer) { std::cout << "[Peer] Connected to routing server at " << ip << '\n'; }
                else {
                    std::cout << "[Peer] Connected to P2P peer at " << ip << '\n';
                    m_peers.emplace_back(e.peer);
                    m_connected_peers.emplace_back(e.peer);
                }
            }
            break;
            case ENET_EVENT_TYPE_DISCONNECT: {
                std::cout << "[Peer] Disconnected from " << ip << '\n';
                if (e.peer == m_server_peer) { m_server_peer = nullptr; }
                else if (auto it = std::ranges::find(m_peers, e.peer);
                    it != m_peers.end()) {
                    m_peers.erase(it);
                    m_disconnected_peers.emplace_back(e.peer);
                }
            }
            break;
            case ENET_EVENT_TYPE_RECEIVE: {
                flatbuffers::Verifier verifier(e.packet->data, e.packet->dataLength);
                if (!fb::VerifyEnvelopeBuffer(verifier)) {
                    std::cout << "[Peer] Received invalid packet (" << e.packet->dataLength << " bytes)\n";
                    enet_packet_destroy(e.packet);
                    break;
                }
                const fb::Envelope* envelope = fb::GetEnvelope(e.packet->data);
                const fb::Message msg_type = envelope->message_type();

                if (msg_type == fb::Message_PeerList) {
                    const fb::PeerList* peer_list = envelope->message_as_PeerList();
                    if (peer_list && peer_list->peers()) {
                        for (const fb::PeerAddress* pa: *peer_list->peers()) {
                            ENetAddress addr{pa->host(), pa->port()};
                            char peer_ip[40];
                            enet_address_get_host_ip(&addr, peer_ip, 40);
                            std::cout << "[Peer] Connecting to listed peer "
                                << peer_ip << ':' << pa->port() << '\n';
                            if (!enet_host_connect(m_host, &addr, 2, 0))
                                std::cerr << "[Peer] No peer slots for " << peer_ip << '\n';
                        }
                        enet_host_flush(m_host);
                    }
                }
                else if (msg_type == fb::Message_NewPeer) {
                    const fb::NewPeer* new_peer = envelope->message_as_NewPeer();
                    if (new_peer && new_peer->address()) {
                        ENetAddress addr{new_peer->address()->host(), new_peer->address()->port()};
                        char peer_ip[40];
                        enet_address_get_host_ip(&addr, peer_ip, 40);
                        std::cout << "[Peer] New peer introduced: "
                            << peer_ip << ':' << new_peer->address()->port() << '\n';
                        if (!enet_host_connect(m_host, &addr, 2, 0))
                            std::cerr << "[Peer] No peer slots for " << peer_ip << '\n';
                        enet_host_flush(m_host);
                    }
                }
                else {
                    incoming_msg msg;
                    msg.from = e.peer;
                    msg.data.assign(
                        e.packet->data,
                        e.packet->data + e.packet->dataLength
                    );
                    m_inbox.emplace_back(std::move(msg));
                }
                enet_packet_destroy(e.packet);
            }
            break;
            default:
                break;
            }
        }
    }

    bool peer::is_live() const { return m_peers.size() != 0; }

    std::array<int, 4> ip_into_octets(const uint32_t ip) {
        const auto* b = reinterpret_cast<const uint8_t*>(&ip);
        return {b[0], b[1], b[2], b[3]};
    }

    uint32_t octets_into_ip(const std::array<int, 4> octets) {
        uint32_t result;
        auto* b = reinterpret_cast<uint8_t*>(&result);
        b[0] = static_cast<uint8_t>(octets[0]);
        b[1] = static_cast<uint8_t>(octets[1]);
        b[2] = static_cast<uint8_t>(octets[2]);
        b[3] = static_cast<uint8_t>(octets[3]);
        return result;
    }
} // namespace Core
