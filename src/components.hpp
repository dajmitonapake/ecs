#pragma once

#include "blob_vector.hpp"

#include <cstdint>
#include <typeindex>
#include <unordered_map>

using component_id = std::uint64_t;

class Components {
public:
    template<typename T>
    component_id registerComponent() {
        auto typeIdx = std::type_index(typeid(T));

        assert(_componentMap.find(typeIdx) == _componentMap.end());

        component_id id = nextId();
        TypeInfo info = TypeInfo::Of<T>();

        _componentMap[typeIdx] = id;
        _types[id] = info;

        return id;
    }

    component_id registerComponent(TypeInfo typeInfo) {
        component_id id = nextId();

        _types[id] = typeInfo;
        return id;
    }

    template<typename T>
    component_id getId() const {
        auto it = _componentMap.find(std::type_index(typeid(T)));

        assert(it != _componentMap.end() && "Component type is not registered!");
        return it->second;
    }

    template<typename T>
    bool isRegistered() const {
        return _componentMap.find(std::type_index(typeid(T))) != _componentMap.end();
    }

    bool isRegistered(component_id id) const {
        return _types.find(id) != _types.end();
    }

    template<typename T>
    TypeInfo getTypeInfo() const {
        return getTypeInfo(getId<T>());
    }

    TypeInfo getTypeInfo(component_id id) const {
        auto it = _types.find(id);

        assert(it != _types.end());
        return it->second;
    }

private:
    component_id nextId() {
        assert(_nextId != 0);

        component_id id = _nextId;
        _nextId <<= 1;
        return id;
    }

    std::unordered_map<component_id, TypeInfo> _types;
    std::unordered_map<std::type_index, component_id> _componentMap;
    component_id _nextId = 1;
};
