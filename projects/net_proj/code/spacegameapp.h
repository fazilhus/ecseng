#pragma once
//------------------------------------------------------------------------------
/**
	Space game application

	(C) 20222 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/app.h"
#include "../../../engine/net/net.h"
#include "core/world.h"
#include "render/window.h"


namespace Game {
    class SpaceGameApp : public Core::App {
    public:
        /// constructor
        SpaceGameApp();
        /// destructor
        virtual ~SpaceGameApp() override;

        /// open app
        virtual bool Open() override;
        /// run app
        virtual void Run() override;
        /// exit app
        virtual void Exit() override;

    private:
        /// show some ui things
        void RenderUI();

        Display::Window* window;
        Ecs::World* world;
        Core::peer peer;
        Core::server server;
        uint32_t ip;
        uint16_t port;
    };
} // namespace Game
