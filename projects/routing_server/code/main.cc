//------------------------------------------------------------------------------
// main.cc
// Headless P2P routing server.
// Listens for peer connections, introduces peers to each other.
//------------------------------------------------------------------------------
#include "net.h"
#include "test_client.h"
#include "firewall.h"

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif


static std::atomic<bool> g_running{true};

static void signal_handler(int) {
    g_running = false;
}


int main(int argc, char* argv[]) {
    uint16_t port = 6969;

    // --test mode: run a headless smoke test against a server on the given port.
    // Usage: p2p_routing_proj --test [port]
    if (argc > 1 && std::strcmp(argv[1], "--test") == 0) {
        if (argc > 2) port = static_cast<uint16_t>(std::atoi(argv[2]));
        return run_smoke_test(port);
    }

    if (argc > 1) {
        const auto parsed = std::atoi(argv[1]);
        if (parsed <= 0 || parsed > 65535) {
            std::cerr << "Invalid port number: " << argv[1] << " (must be 1-65535)\n";
            return 1;
        }
        port = static_cast<uint16_t>(parsed);
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Allow inbound UDP through Windows Firewall (LAN play)
    Core::ensure_udp_firewall_rule("ECS RoutingServer");

    Core::server srv;
    if (!srv.init(port)) {
        std::cerr << "[RoutingServer] Failed to initialize on port " << port << '\n';
        return 1;
    }

    std::cout << "[RoutingServer] Listening on port " << port << '\n';
    std::cout << "[RoutingServer] Press Ctrl+C to stop.\n";

    // LAN discovery: broadcast beacon every 2 s on UDP 6970
    std::thread disc_thread([&]() {
#ifdef _WIN32
        SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s == INVALID_SOCKET) return;
        BOOL yes = TRUE;
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_port   = htons(6970);
        dst.sin_addr.s_addr = INADDR_BROADCAST;
        const char* beacon = "ECS1";
        while (g_running.load()) {
            sendto(s, beacon, 4, 0, (sockaddr*)&dst, sizeof(dst));
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        closesocket(s);
#endif
    });

    while (g_running.load()) {
        srv.update();
    }

    std::cout << "\n[RoutingServer] Shutting down...\n";
    if (disc_thread.joinable()) disc_thread.join();
    srv.deinit();

    return 0;
}
