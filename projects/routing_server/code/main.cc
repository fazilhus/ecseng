//------------------------------------------------------------------------------
// main.cc
// Headless P2P routing server.
// Listens for peer connections, introduces peers to each other.
//------------------------------------------------------------------------------
#include "net.h"
#include "test_client.h"

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>


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

    Core::server srv;
    if (!srv.init(port)) {
        std::cerr << "[RoutingServer] Failed to initialize on port " << port << '\n';
        return 1;
    }

    std::cout << "[RoutingServer] Listening on port " << port << '\n';
    std::cout << "[RoutingServer] Press Ctrl+C to stop.\n";

    while (g_running.load()) {
        srv.update();
    }

    std::cout << "\n[RoutingServer] Shutting down...\n";
    srv.deinit();

    return 0;
}
