#pragma once

#include <cstdint>
#include <bitset>

namespace Ecs {

	using EntityID = std::uint32_t;
	constexpr EntityID MaxEntityCount = 1000;

	using ComponentID = std::uint8_t;
	constexpr ComponentID MaxComponentCount = 32;
	
	using Signature = std::bitset<MaxComponentCount>;

} // namespace Ecs
