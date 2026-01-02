#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

using EntityId = std::size_t;
using EntityGeneration = std::uint16_t;

struct Entity {
    EntityId id;
    EntityGeneration generation;
};

struct EntityLocation {
    std::size_t archetype;
    std::size_t row;
};

struct EntityMeta {
    EntityGeneration generation;
    std::optional<EntityLocation> location;
};

class Entities {
public:
    Entity create();

    void setLocation(Entity entity, EntityLocation location);
    void despawn(Entity entity);

    bool isEmpty(Entity entity) const;
    bool isAlive(Entity entity) const;
    std::optional<EntityLocation> getLocation(Entity entity) const;

private:
    std::vector<EntityMeta> entities;
    std::vector<std::size_t> free;
};
