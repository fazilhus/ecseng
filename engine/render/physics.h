#pragma once
//------------------------------------------------------------------------------
/**
    @file physics.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <string>

namespace Physics
{
    
struct ColliderId
{
    uint32_t index : 22; // 4M concurrent colliders
    uint32_t generation : 10; // 1024 generations per index

    constexpr static ColliderId Create(uint32_t id)
    {
        ColliderId ret { id & 0x003FFFFF, (id & 0xFFC00000) >> 22 };
        return ret;
    }
    static ColliderId Create(uint32_t index, uint32_t generation)
    {
        ColliderId ret;
        ret.index = index;
        ret.generation = generation;
        return ret;
    }
    explicit constexpr operator uint32_t() const
    {
        return ((generation << 22) & 0xFFC00000ul) + (index & 0x003FFFFFul);
    }
    static constexpr ColliderId Invalid()
    {
        return Create(0xFFFFFFFF);
    }
    constexpr uint32_t HashCode() const
    {
        return index;
    }
    const bool operator==(const ColliderId& rhs) const { return uint32_t(*this) == uint32_t(rhs); }
    const bool operator!=(const ColliderId& rhs) const { return uint32_t(*this) != uint32_t(rhs); }
    const bool operator<(const ColliderId& rhs) const { return index < rhs.index; }
    const bool operator>(const ColliderId& rhs) const { return index > rhs.index; }
};

struct ColliderMeshId
{
    uint32_t index : 22; // 4M concurrent meshes
    uint32_t generation : 10; // 1024 generations per index

    constexpr static ColliderMeshId Create(uint32_t id)
    {
        ColliderMeshId ret { id & 0x003FFFFF, (id & 0xFFC00000) >> 22 };
        return ret;
    }
    explicit constexpr operator uint32_t() const
    {
        return ((generation << 22) & 0xFFC00000ul) + (index & 0x003FFFFFul);
    }
    static constexpr ColliderMeshId Invalid()
    {
        return Create(0xFFFFFFFF);
    }
    constexpr uint32_t HashCode() const
    {
        return index;
    }
    const bool operator==(const ColliderMeshId& rhs) const { return uint32_t(*this) == uint32_t(rhs); }
    const bool operator!=(const ColliderMeshId& rhs) const { return uint32_t(*this) != uint32_t(rhs); }
    const bool operator<(const ColliderMeshId& rhs) const { return index < rhs.index; }
    const bool operator>(const ColliderMeshId& rhs) const { return index > rhs.index; }
};

struct RaycastPayload
{
    bool hit = false;
    float hitDistance = 0;
    glm::vec3 hitPoint;
    ColliderId collider;
};

RaycastPayload Raycast(glm::vec3 start, glm::vec3 dir, float maxDistance, uint16_t mask = 0);

ColliderId CreateCollider(ColliderMeshId meshId, glm::mat4 const& transform, uint16_t mask = 0, void* userData = nullptr);

ColliderMeshId LoadColliderMesh(std::string path);

void SetTransform(ColliderId collider, glm::mat4 const& transform);

} // namespace Physics
