#include "../query.hpp"
#include "../world.hpp"

extern "C" {
    Query* _QueryCreate() {
        return std::make_unique<Query>().release();
    }

    int _QueryIter(World* world, component_id fetchBitmask, Query* query, QueryChunk** outChunks) {
        query->fetch(&world->archetypes, fetchBitmask);
        *outChunks = query->chunks.data();
        return static_cast<int>(query->chunks.size());
    }

    void _QueryDestroy(Query* query) {
        std::unique_ptr<Query> _(query);
    }
}
