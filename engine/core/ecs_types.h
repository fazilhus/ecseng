#pragma once

#include <cstdint>
#include <bitset>


namespace Ecs {
    using EntityID = std::uint32_t;
    constexpr EntityID MaxEntityCount = 1000;

    using ComponentID = std::uint8_t;
    constexpr ComponentID MaxComponentCount = 32;

    enum ComponentTypes {
        CT_TRANSFORM = 1 << 1,
        CT_CAMERA = 1 << 2,
        CT_MODEL = 1 << 3,
        CT_COLLIDER = 1 << 4,
        CT_MOVEMENT = 1 << 5,
        CT_PLAYER_CHARACTER = 1 << 6,
        CT_AI_CHARACTER = 1 << 7,
        CT_COLLISION = 1 << 8,
        CT_WAYPOINT = 1 << 9,
        CT_PARTICLE_EMITTER = 1 << 10,
        CT_PROJECTILE_SPAWNER = 1 << 11,
        CT_PROJECTILE = 1 << 12,
        CT_MAX = 1 << 31,
    };

#define CT_START CT_TRANSFORM

    using Signature = std::uint32_t;
} // namespace Ecs

enum BehaviourType {
    BT_Neutral,
    BT_Aggressive,
    BT_Defensive,
};
enum StateType {
    ST_Moving,
    ST_Acting,
};