#include "query.hpp"
#include "archetype.hpp"

#include <bit>

void Query::fetch(Archetypes* archetypes, component_id fetchBitmask) {
    this->chunks.clear();
    this->columns.clear();

    for (auto& archetype : archetypes->archetypes()) {
        if ((archetype.bitmask() & fetchBitmask) == fetchBitmask) {
            size_t poolStartIndex = this->columns.size();

            auto mask = fetchBitmask;

            for (std::size_t i = 0; i < std::popcount(fetchBitmask); ++i) {
                if (mask == 0) break;
                auto index = std::countr_zero(mask);
                auto bit = component_id(1) << index;

                if (fetchBitmask & bit) {
                    this->columns.push_back({ (std::byte*)archetype.getColumn(bit)->data() });
                }

                mask ^= bit;
            }

            chunks.push_back({ &this->columns[poolStartIndex], archetype.entityData(), archetype.length() });
        }
    }
}
