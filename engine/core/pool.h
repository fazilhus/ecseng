#pragma once
#include <vector>

#include "core/entity.h"

namespace Ecs {

	template<typename Component>
	class ComponentPool {
		EntityID* sparse;
		EntityID* dense;
		std::vector<Component> comps;

		EntityID size;
		EntityID cap;

	public:
		ComponentPool();
		~ComponentPool();

		bool Has(EntityID id) const;

		const Component& GetComponent(EntityID id) const;

		template <typename ...Args>
		void Add(EntityID id, Args... args);
		void Remove(EntityID id);
		void Clear();

		Iterator begin() { return Iterator(&dense[0]); }
		Iterator end() { return Iterator(&dense[n]); }

		struct Iterator {
			using iterator_category = std::random_access_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = EntityID;
			using pointer = value_type*;
			using reference = value_type&;

			Iterator(pointer ptr) : ptr(ptr) {}

			Iterator

		private:
			pointer ptr;
		};
	};

	template<typename Component>
	inline ComponentPool<Component>::ComponentPool() : size(0), cap(MaxEntityCount) {
		sparse = new EntityID[cap];
		dense = new EntityID[cap];
		comps.reserve(cap);
	}

	template<typename Component>
	inline ComponentPool<Component>::~ComponentPool() {
		delete[] sparse;
		delete[] dense;
	}

	template<typename Component>
	inline bool ComponentPool<Component>::Has(EntityID id) const {
		return id < cap && id < size && dense[sparse[id]] == id;
	}

	template<typename Component>
	inline const Component& ComponentPool<Component>::GetComponent(EntityID id) const {
		if (!Has(id)) return nullptr;
		return comps[sparse[id]];
	}

	template<typename Component>
	template<typename ...Args>
	inline void ComponentPool<Component>::Add(EntityID id, Args ...args) {
		if (Has(id)) return;

		dense[size] = id;
		comps.emplace_back(std::forward<Args>(args)...);
		sparse[id] = size++;
	}

	template<typename Component>
	inline void ComponentPool<Component>::Remove(EntityID id) {
		if (!Has(id)) return;

		EntityID di = sparse[id];
		EntityID si = dense[di];

		if (di == size - 1) {
			size--;
			return;
		}

		std::swap(dense[di], dense[size - 1]);
		std::swap(comps[di], comps.back());
		sparse[si] = di;
		size--;
	}

	template<typename Component>
	inline void ComponentPool<Component>::Clear() {
		size = 0;
	}

} // namespace Mem