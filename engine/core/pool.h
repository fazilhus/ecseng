#pragma once
#include <vector>

#include "core/ecs_types.h"


namespace Ecs {
    class BaseComponentPool {
    public:
        virtual ~BaseComponentPool() {}

        virtual bool Remove(EntityID id) { return false; }
    };

    template <typename Comp>
    class ComponentPool : public BaseComponentPool {
        EntityID* sparse;
        EntityID* dense;
        std::vector<Comp> comps;

        EntityID size;
        EntityID cap;

    public:
        ComponentPool();
        ~ComponentPool() override;

        bool Has(EntityID id) const;

        const Comp& Get(EntityID id) const;
        Comp& Get(EntityID id);

        template <typename ...Args>
        void Add(EntityID id, Args ...args);
        bool Remove(EntityID id) override;
        void Clear();

        struct Iterator {
            using iterator_category = std::random_access_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = EntityID;
            using pointer = value_type*;
            using reference = value_type&;

            Iterator(pointer ptr)
                : ptr(ptr) {}

            //Iterator

        private:
            pointer ptr;
        };

        Iterator begin() { return Iterator(&this->dense[0]); }
        Iterator end() { return Iterator(&this->dense[this->size]); }
    };

    template <typename Comp>
    inline ComponentPool<Comp>::ComponentPool()
        : size(0), cap(MaxEntityCount) {
        sparse = new EntityID[cap];
        dense = new EntityID[cap];
        comps.reserve(cap);
    }

    template <typename Comp>
    inline ComponentPool<Comp>::~ComponentPool() {
        delete[] sparse;
        delete[] dense;
    }

    template <typename Comp>
    inline bool ComponentPool<Comp>::Has(EntityID id) const {
        return id < cap && sparse[id] < size && dense[sparse[id]] == id;
    }

    template <typename Comp>
    inline const Comp& ComponentPool<Comp>::Get(EntityID id) const {
        if (!Has(id))
            assert(false && "oopser daiser");
        return comps[sparse[id]];
    }

    template <typename Comp>
    inline Comp& ComponentPool<Comp>::Get(EntityID id) { return comps[sparse[id]]; }

    template <typename Comp>
    template <typename ...Args>
    inline void ComponentPool<Comp>::Add(EntityID id, Args ...args) {
        if (Has(id))
            return;

        dense[size] = id;
        comps.emplace_back(std::forward<Args>(args) ...);
        sparse[id] = size++;
    }

    template <typename Comp>
    inline bool ComponentPool<Comp>::Remove(EntityID id) {
        if (!Has(id))
            return false;

        EntityID di = sparse[id];
        EntityID si = dense[di];

        if (di == size - 1) {
            size--;
            return true;
        }

        std::swap(dense[di], dense[size - 1]);
        std::swap(comps[di], comps.back());
        sparse[si] = di;
        size--;
        return true;
    }

    template <typename Comp>
    inline void ComponentPool<Comp>::Clear() { size = 0; }
} // namespace Mem
