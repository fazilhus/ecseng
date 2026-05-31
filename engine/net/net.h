#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>
#include <enet/enet.h>

#include "protocol.h"

inline std::ostream& operator<<(std::ostream& os, std::array<int, 4> ip_octets) {
    os << ip_octets[0] << '.' << ip_octets[1] << '.' << ip_octets[2] << '.' << ip_octets[3];
    return os;
}

namespace Core {

    // constexpr uint32_t g_routing_server_address = 0x7f000001;
    // constexpr uint16_t g_routing_server_port = 6969;

    class server {
    public:
        explicit server();
        ~server();

        bool init(uint16_t port);
        void deinit();

        void update();

        bool m_initialized;
        uint16_t m_port;
        ENetHost* m_host;
        std::vector<ENetPeer*> m_peers;
    };

    class peer {
    public:
        explicit peer();
        ~peer();

        bool init();
        void deinit();

        bool connect(uint32_t ip, uint16_t port);
        void disconnect();

        void update();

        bool m_initialized;
        ENetHost* m_host;
        ENetPeer* m_server_peer;
        std::vector<ENetPeer*> m_peers;
    };

    std::array<int, 4> ip_into_octets(uint32_t ip);
    uint32_t octets_into_ip(std::array<int, 4> octets);

} // core