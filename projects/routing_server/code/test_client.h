//------------------------------------------------------------------------------
// test_client.h
// Headless FlatBuffers smoke test.
//
// Self-contained: starts a Core::server in a background thread, then connects
// a raw-ENet client and verifies the first packet is a valid fb::Envelope with
// Message_PeerList.  No external process required.
//
// Returns 0 on success, 1 on failure.
//------------------------------------------------------------------------------
#pragma once
#include "net.h"
#include "protocol.h"
#include <atomic>
#include <iostream>
#include <thread>
#include <enet/enet.h>

inline int run_smoke_test(uint16_t port) {
    // -----------------------------------------------------------------------
    // 1. Try to start a routing server in a background thread.
    //    If the port is already in use (an external server is running) we
    //    skip the self-hosted server and connect to the external one instead.
    //    Core::server::init() calls enet_initialize() internally; matched by
    //    srv.deinit() at the end (only when we own the server).
    // -----------------------------------------------------------------------
    Core::server srv;
    const bool own_server = srv.init(port);
    if (!own_server)
        std::cout << "[TEST] Port " << port
                  << " already bound — connecting to external routing server\n";

    std::atomic<bool> srv_stop{false};
    std::thread srv_thread([&]() {
        if (!own_server) return;                         // nothing to do
        while (!srv_stop.load(std::memory_order_relaxed))
            srv.update();   // blocks ≤32 ms per iteration waiting for events
    });

    // -----------------------------------------------------------------------
    // 2. Client side: raw ENet with its own enet_initialize / enet_deinitialize.
    //    Running on a separate ENetHost*, safe to use from this thread while
    //    the server thread uses its own ENetHost*.
    // -----------------------------------------------------------------------
    int result = 1;

    if (enet_initialize() != 0) {
        std::cerr << "[TEST] Failed to initialize ENet for client\n";
        goto teardown_server;
    }

    {
        ENetAddress addr{};
        enet_address_set_host(&addr, "127.0.0.1");
        addr.port = port;

        ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!client) {
            std::cerr << "[TEST] Failed to create ENet client host\n";
            goto teardown_enet;
        }

        ENetPeer* peer = enet_host_connect(client, &addr, 2, 0);
        if (!peer) {
            std::cerr << "[TEST] Failed to initiate connection\n";
            enet_host_destroy(client);
            goto teardown_enet;
        }

        {
            bool connected     = false;
            bool got_peer_list = false;
            result = 0;

            ENetEvent e;
            while (!got_peer_list && enet_host_service(client, &e, 3000) > 0) {
                if (e.type == ENET_EVENT_TYPE_CONNECT) {
                    std::cout << "[TEST] Connected to routing server on port " << port << '\n';
                    connected = true;

                } else if (e.type == ENET_EVENT_TYPE_RECEIVE) {
                    flatbuffers::Verifier verifier(
                        reinterpret_cast<const uint8_t*>(e.packet->data),
                        e.packet->dataLength);
                    if (!fb::VerifyEnvelopeBuffer(verifier)) {
                        std::cerr << "[TEST] FAIL: packet is not a valid FlatBuffer Envelope\n";
                        enet_packet_destroy(e.packet);
                        result = 1;
                        break;
                    }
                    const fb::Envelope* envelope = fb::GetEnvelope(e.packet->data);
                    if (envelope->message_type() == fb::Message_PeerList) {
                        const fb::PeerList* pl    = envelope->message_as_PeerList();
                        const uint32_t      count = (pl && pl->peers()) ? pl->peers()->size() : 0u;
                        std::cout << "[TEST] PASS: valid PeerList received, "
                                  << count << " existing peers\n";
                        got_peer_list = true;
                    } else {
                        std::cerr << "[TEST] FAIL: expected Message_PeerList, got "
                                  << fb::EnumNameMessage(envelope->message_type()) << '\n';
                        result = 1;
                    }
                    enet_packet_destroy(e.packet);

                } else if (e.type == ENET_EVENT_TYPE_DISCONNECT) {
                    std::cerr << "[TEST] FAIL: disconnected before receiving PeerList\n";
                    result = 1;
                    break;
                }
            }

            if (!connected)                         { std::cerr << "[TEST] FAIL: never connected to server\n";  result = 1; }
            else if (!got_peer_list && result == 0) { std::cerr << "[TEST] FAIL: never received PeerList\n";    result = 1; }
        }

        enet_peer_disconnect_now(peer, 0);
        enet_host_flush(client);
        enet_host_destroy(client);
    }

teardown_enet:
    enet_deinitialize();

teardown_server:
    srv_stop = true;
    srv_thread.join();
    if (own_server) srv.deinit();

    return result;
}
