#pragma once
#include <vector>

namespace Mem {

	using EntityId = std::uint32_t;
	using ComponentId = std::uint32_t;

	template <typename T>
	class ComponentPool {
	public:
		ComponentPool(std::size_t entity_number, std::size_t component_number);
		~ComponentPool() = default;

	private:
		std::vector<std::size_t> m_indices;
		std::vector<T> m_component_list;
	};

} // namespace Mem