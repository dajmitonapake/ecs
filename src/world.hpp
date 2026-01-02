#pragma once

#include "bundle.hpp"
#include "components.hpp"
#include "entity.hpp"
#include "archetype.hpp"
#include "query.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>

class World {
public:
    Entities entities;
    Archetypes archetypes;
    std::shared_ptr<Components> components;

public:
    explicit World();

    component_id registerComponent(const TypeInfo typeInfo) {
        return this->components->registerComponent(typeInfo);
    }

    template<typename T>
    component_id registerComponent() {
        auto id = components->registerComponent<T>();
        return id;
    }

    template<typename T>
    component_id getComponentId() const {
        return components->getId<T>();
    }

    Entity spawnEmpty();
    Entity spawnBundle(std::unique_ptr<Bundle> bundle);

    template<typename... Components>
    Entity spawn(Components&&... components) {
        auto bundle = this->createBundle(components...);
        auto entity = this->spawnEmpty();

        this->insertBundle(entity, std::move(bundle));

        return entity;
    }

    void insertBundle(Entity entity, std::unique_ptr<Bundle> bundle);

    template<typename... Components>
    void insert(Entity entity, Components&&... components) {
        if (!this->entities.isAlive(entity)) {
            throw std::runtime_error("Entity is not alive while trying to insert components");
        }

        auto bundle = this->createBundle(components...);
        this->insertBundle(entity, std::move(bundle));
    }

    void removeBundle(Entity entity, std::unique_ptr<Bundle> bundle);

    template<typename... Components>
    void remove(Entity entity) {
        if (!this->entities.isAlive(entity)) {
            throw std::runtime_error("Entity is not alive while trying to remove components");
        }

        auto bundle = std::make_unique<Bundle>(this->createBitmask<Components...>());
        this->removeBundle(entity, std::move(bundle));
    }

    std::byte* get(Entity entity, component_id componentId);

    template<typename T>
    T* get(Entity entity) {
        if (!this->entities.isAlive(entity)) {
            throw std::runtime_error("Entity is not alive while trying to get component");
        }

        auto location = this->entities.getLocation(entity).value();
        auto archetype = this->archetypes.at(location.archetype);
        auto column = archetype->getColumn(components->getId<T>());
        return column->template Get<T>(location.row);
    }

    void despawn(Entity entity);

    template<typename... Comps, typename Func>
    void iter( Func&& func) {
        auto query = Query();
        auto bitmask = this->createBitmask<Comps...>();
        query.fetch(&this->archetypes, bitmask);
        query.template iterate<Comps...>(std::forward<Func>(func), std::make_index_sequence<sizeof...(Comps)>{});
    }

private:

    template<typename... Components>
    std::unique_ptr<Bundle> createBundle(Components&&... components) const {
        std::size_t totalSize = (sizeof(Components) + ...);
        std::byte* buffer = static_cast<std::byte*>(::operator new(totalSize));
        std::size_t offset = 0;

        (..., (
            new (buffer + offset) std::decay_t<Components>(std::forward<Components>(components)),
            offset += sizeof(Components)
        ));

        component_id mask = this->createBitmask<Components...>();

        auto bundle = std::make_unique<Bundle>(this->components, mask, buffer, sizeof...(Components), true);

        return bundle;
    }

    template<typename... Components>
    component_id createBitmask() const {
        return (
            (std::is_same_v<std::decay_t<Components>, Entity> ? 0
                : this->getComponentId<std::decay_t<Components>>())
            | ... | 0
        );
    }
};
