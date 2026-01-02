#include "archetype.hpp"
#include "blob_vector.hpp"

#include <bit>
#include <cassert>
#include <print>
#include <unordered_map>

Archetype::Archetype(component_id bitmask, std::shared_ptr<Components> components)  {
    this->_bitmask = bitmask;
    this->_components = components;
    this->_columns.reserve(std::popcount(bitmask));

    while (bitmask != 0) {
        auto index = std::countr_zero(bitmask);
        auto bit = component_id(1) << index;

        if ((bitmask & bit) != 0) {
            assert(this->_components->isRegistered(bit));

            auto typeInfo = this->_components->getTypeInfo(bit);
            this->addColumn(bit, std::move(typeInfo));
        }

        bitmask ^= bit;
    }
}

void Archetype::push(component_id bit, std::byte* bytes) {
    assert(this->_columnMap.contains(bit));

    auto position = this->_columnMap[bit];
    this->_columns[position].push(bytes);
}

void Archetype::moveData(std::size_t row, Archetype* to) {
    auto last = this->length() - 1;

    for (const auto& [bit, pos] : this->_columnMap) {
        auto& src = this->_columns[pos];

        if ((to->bitmask() & bit) != 0) {
            auto dst = to->getColumn(bit);
            assert(dst != nullptr);

            dst->set(dst->length() - 1, src.get(row));
        } else {
            src.typeInfo().destructor(src.get(row));
        }
    }

    if (row != last) {
        for (auto& column : this->_columns) {
            column.typeInfo().destructor(column.swapRemove(row));
        }
    }

}

void Archetype::addColumn(component_id bit, TypeInfo typeInfo) {
    this->_columnMap[bit] = this->_columns.size();
    this->_columns.emplace_back<BlobVector>(std::move(typeInfo));
}

void Archetype::grow(Entity entity) {
    this->_entities.push_back(std::move(entity));

    for (auto& column : this->_columns) {
        column.grow(1);
    }
}

void Archetype::setEntity(std::size_t row, Entity entity) {
    assert(row < this->_entities.size());

    this->_entities[row] = std::move(entity);
}

void Archetype::popEntity() {
    assert(this->_entities.size() > 0);

    this->_entities.pop_back();
}

Entity* Archetype::entityData() {
    return this->_entities.data();
}

Entity Archetype::getEntity(std::size_t row) {
    assert(row < this->_entities.size());

    return this->_entities[row];
}

BlobVector* Archetype::getColumn(component_id bit) {
    if (!this->_columnMap.contains(bit)) {
        return nullptr;
    }

    return &this->_columns[this->_columnMap[bit]];
}

std::size_t Archetype::length() const {
    return this->_entities.size();
}

component_id Archetype::bitmask() const {
    return this->_bitmask;
}


Archetypes::Archetypes(std::shared_ptr<Components> components) {
    this->_components = components;
}

void Archetypes::add(component_id bitmask, Archetype&& archetype) {
    this->_archetypeMap[bitmask] = this->_archetypes.size();
    this->_archetypes.emplace_back(std::move(archetype));
}

void Archetypes::moveEntity(Entity entity, component_id from, component_id to, Entities* entities){
    auto toIndex = this->_archetypeMap[to];

    auto fromArchetype = this->get(from);
    auto toArchetype = this->get(to);

    toArchetype->grow(std::move(entity));

    auto oldLocation = entities->getLocation(entity).value();
    auto lastIndex = fromArchetype->length() - 1;

    fromArchetype->moveData(oldLocation.row, toArchetype);

    if (oldLocation.row != lastIndex) {
        auto lastEntity = fromArchetype->getEntity(lastIndex);
        fromArchetype->setEntity(oldLocation.row, lastEntity);

        entities->setLocation(lastEntity, oldLocation);
    }

    fromArchetype->popEntity();

    auto newRow = toArchetype->length() - 1;
    entities->setLocation(entity, EntityLocation {toIndex, newRow});
}

Archetype* Archetypes::getOrCreate(component_id bit) {
    if (!this->_archetypeMap.contains(bit)) {
        auto archetype = Archetype(bit, this->_components);
        this->add(bit, std::move(archetype));
    }

    return this->get(bit);
}

Archetype* Archetypes::get(component_id bit) {
    if (!this->_archetypeMap.contains(bit)) {
        return nullptr;
    }

    return &this->_archetypes[this->_archetypeMap[bit]];
}


Archetype* Archetypes::at(std::size_t index) {
    if (index >= this->_archetypes.size()) {
        return nullptr;
    }

    return &this->_archetypes[index];
}

std::deque<Archetype>& Archetypes::archetypes() {
    return this->_archetypes;
}

std::size_t Archetypes::position(component_id bit) const {
    assert(this->_archetypeMap.contains(bit));

    return this->_archetypeMap.at(bit);
}

bool Archetypes::exists(component_id bit) const {
    return this->_archetypeMap.contains(bit);
}

std::size_t Archetypes::length() const {
    return this->_archetypes.size();
}
