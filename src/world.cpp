#include "world.hpp"
#include "entity.hpp"

#include <print>

World::World() {
    this->components = std::make_shared<Components>();
    this->entities = Entities();
    this->archetypes = Archetypes(this->components);
}

std::byte* World::get(Entity entity, component_id componentId) {
    auto location = this->entities.getLocation(entity).value();
    auto archetype = this->archetypes.at(location.archetype);
    auto column = archetype->getColumn(componentId);
    return column->get(location.row);
}

Entity World::spawnEmpty() {
    auto entity = this->entities.create();
    return entity;
}

Entity World::spawnBundle(std::unique_ptr<Bundle> bundle) {
    auto entity = this->entities.create();
    this->insertBundle(entity, std::move(bundle));
    return entity;
}

void World::insertBundle(Entity entity, std::unique_ptr<Bundle> bundle) {
    auto oldLocation = this->entities.getLocation(entity);
    auto oldBitmask = 0;

    if (oldLocation.has_value()) {
        oldBitmask = this->archetypes.at(oldLocation.value().archetype)->bitmask();
    }

    auto targetBitmask = oldBitmask | bundle->bitmask;
    auto targetArchetype = this->archetypes.getOrCreate(targetBitmask);

    if (targetBitmask != oldBitmask) {
        if (!this->entities.isEmpty(entity)) {
            this->archetypes.moveEntity(entity, oldBitmask, targetBitmask, &this->entities);
        } else {
            auto target = this->archetypes.get(targetBitmask);
            auto targetPosition = this->archetypes.position(targetBitmask);

            target->grow(entity);

            auto row = target->length() - 1;
            this->entities.setLocation(entity, EntityLocation{targetPosition, row});
        }
    }

    auto targetLocation = this->entities.getLocation(entity).value();

    bundle->transfer([&](component_id bit, std::byte* bytes) {
        auto targetColumn = targetArchetype->getColumn(bit);

        if ((oldBitmask & bit) != 0) {
            targetColumn->replace(targetLocation.row, bytes);
        } else {
            targetColumn->set(targetLocation.row, bytes);
        }
    });
}

void World::removeBundle(Entity entity, std::unique_ptr<Bundle> bundle) {
    auto oldLocation = this->entities.getLocation(entity);

    if (!oldLocation.has_value()) {
        return;
    }

    auto oldBitmask = this->archetypes.at(oldLocation.value().archetype)->bitmask();
    auto targetBitmask = oldBitmask & ~bundle->bitmask;

    this->archetypes.getOrCreate(targetBitmask);
    this->archetypes.moveEntity(entity, oldBitmask, targetBitmask, &this->entities);
}


void World::despawn(Entity entity) {
    if (!this->entities.isAlive(entity)) {
        throw std::runtime_error("Entity is not alive while trying to despawn it");
    }

    auto location = this->entities.getLocation(entity);
    if (!location.has_value()) {
        return;
    }
    auto archetype = this->archetypes.at(location.value().archetype);
    auto bitmask = archetype->bitmask();

    this->removeBundle(entity, std::make_unique<Bundle>(bitmask));
    this->entities.despawn(entity);
}
