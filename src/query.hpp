#pragma once

#include "archetype.hpp"

#include <cstddef>
#include <print>
#include <vector>

// FUTURE
struct QueryCache {
    std::vector<std::size_t> matching;
    component_id highWatermark;
};

struct QueryColumn {
    std::byte* data;
};

struct QueryChunk {
    QueryColumn* columns;
    Entity* entities;
    std::size_t entityCount;
};

class Query {
public:
    std::vector<QueryColumn> columns;
    std::vector<QueryChunk> chunks;

public:
    Query() = default;

    void fetch(Archetypes* archetypes, component_id fetchBitmask);

    template<typename... Comps, typename Func, std::size_t... Is>
    void iterate(Func&& iterator, std::index_sequence<Is...>) {
        for (auto& chunk : chunks) {
            const auto batch_ptrs = std::make_tuple(
                ([&]() {
                    using T = std::tuple_element_t<Is, std::tuple<Comps...>>;

                    if constexpr (std::is_same_v<T, Entity>) {
                        return chunk.entities;
                    } else {
                        return reinterpret_cast<T*>(chunk.columns[Is].data);
                    }
                }())...
            );

            const std::size_t count = chunk.entityCount;
            for (std::size_t i = 0; i < count; ++i) {
                iterator((std::get<Is>(batch_ptrs)[i])...);
            }
        }
    }
};
