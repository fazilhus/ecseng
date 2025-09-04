#pragma once

#include <cstdint>
#include <bitset>


namespace Ecs {
    using EntityID = std::uint32_t;
    constexpr EntityID MaxEntityCount = 1000;

    using ComponentID = std::uint8_t;
    constexpr ComponentID MaxComponentCount = 32;

    enum ComponentTypes {
        CT_TRANSFORM = 1 << 0,
        CT_CAMERA = 1 << 1,
        CT_MESH = 1 << 2,
        CT_MAX = 1 << 31,
    };

#define CT_START CT_TRANSFORM

    using Signature = std::uint32_t;
} // namespace Ecs
