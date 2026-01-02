#pragma once

#include "blob_vector.hpp"
#include "components.hpp"
#include "entity.hpp"

#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

class Archetype {
public:
    explicit Archetype(component_id bitmask, std::shared_ptr<Components> components);

    template<typename T, typename... Args>
    void emplace(Args&&... args) {
        assert(this->_components->isRegistered<T>());

        auto bit = this->_components->getId<T>();
        assert(this->_columnMap.contains(bit));

        this->_columns[this->_columnMap[bit]].template emplace<T>(std::forward<Args>(args)...);
    }

    void push(component_id bit, std::byte* bytes);

    template<TriviallyCopyable T>
    void push(T&& value) {
        assert(this->_components->isRegistered<T>());

        auto bit = this->_components->getId<T>();
        this->push(bit, reinterpret_cast<std::byte*>(&value));
    }

    void moveData(std::size_t row, Archetype* to);
    void addColumn(component_id bit, TypeInfo typeInfo);
    void grow(Entity entity);
    void setEntity(component_id row, Entity entity);
    void popEntity();

    Entity* entityData();
    Entity getEntity(component_id row);
    BlobVector* getColumn(component_id bit);

    std::size_t length() const;
    component_id bitmask() const;

    Archetype(Archetype&&) noexcept = default;
    Archetype& operator=(Archetype&&) noexcept = default;

    Archetype(const Archetype&) = delete;
    Archetype& operator=(const Archetype&) = delete;

    ~Archetype() = default;
private:
    component_id _bitmask;
    std::unordered_map<component_id, std::size_t> _columnMap;
    std::vector<BlobVector> _columns;
    std::vector<Entity> _entities;
    std::shared_ptr<Components> _components;
};

class Archetypes {
public:
    Archetypes() {}
    explicit Archetypes(std::shared_ptr<Components> components);

    void add(component_id bit, Archetype&& archetype);
    void moveEntity(Entity entity, component_id from, component_id to, Entities* entities);

    Archetype* getOrCreate(component_id bit);
    Archetype* get(component_id bit);
    Archetype* at(std::size_t index);
    std::deque<Archetype>& archetypes();

    std::size_t position(component_id bit) const;
    bool exists(component_id bit) const;
    std::size_t length() const;
private:
    std::unordered_map<component_id, std::size_t> _archetypeMap;
    std::deque<Archetype> _archetypes;
    std::shared_ptr<Components> _components;
};
