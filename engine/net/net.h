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

    struct incoming_msg {
        ENetPeer*            from;
        std::vector<uint8_t> data;
    };

    class server {
    public:
        explicit server();
        ~server();

        bool init(uint16_t port);
        void deinit();

        void update();

        bool m_initialized;
        bool m_live;
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

        bool is_live() const;

        bool m_initialized;
        ENetHost* m_host;
        ENetPeer* m_server_peer;

        std::vector<ENetPeer*> m_peers;

        std::vector<incoming_msg> m_inbox;
        std::vector<ENetPeer*>   m_connected_peers;
        std::vector<ENetPeer*>   m_disconnected_peers;
    };

    std::array<int, 4> ip_into_octets(uint32_t ip);
    uint32_t octets_into_ip(std::array<int, 4> octets);

} // core