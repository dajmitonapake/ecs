#include "entity.hpp"

#include <print>

Entity Entities::create() {
    if (!this->free.empty()) {
        auto index = this->free.back();
        auto meta = this->entities[index];
        this->free.pop_back();
        return Entity{index, meta.generation};
    }

    this->entities.emplace_back<EntityMeta>({0, {}});

    return Entity{this->entities.size() - 1, 0};
}

void Entities::setLocation(Entity entity, EntityLocation location) {
    auto& meta = this->entities[entity.id];

    meta.location = location;
}

void Entities::despawn(Entity entity) {
    auto& meta = this->entities[entity.id];
    meta.generation++;
    this->free.emplace_back(entity.id);
}

bool Entities::isEmpty(Entity entity) const {
    return !this->entities[entity.id].location.has_value();
}

bool Entities::isAlive(Entity entity) const {
    return this->entities[entity.id].generation == entity.generation;
}

std::optional<EntityLocation> Entities::getLocation(Entity entity) const {
    return this->entities[entity.id].location;
}
