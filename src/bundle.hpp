#pragma once

#include "components.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <print>

class Bundle {
public:
    component_id bitmask;
public:
    Bundle(component_id bitmask);
    Bundle(std::shared_ptr<Components> components, component_id bitmask, std::byte* data, std::size_t count, bool owned);

    void transfer(std::function<void(component_id, std::byte*)> dest);

    Bundle(Bundle&& other) noexcept
            : bitmask(other.bitmask),
              _components(std::move(other._components)),
              _ownedData(std::move(other._ownedData)),
              _data(std::move(other._data)),
              _count(other._count)
    {
        other.bitmask = 0;
    }

    Bundle& operator=(Bundle&& other) noexcept {
        if (this != &other) {
            this->~Bundle();

            bitmask = std::move(other.bitmask);
            _components = std::move(other._components);
            _ownedData = std::move(other._ownedData);
            _data = std::move(other._data);
            _count = other._count;

            other.bitmask = 0;
        }
        return *this;
    }

    Bundle(const Bundle&) = delete;
    Bundle& operator=(const Bundle&) = delete;

    ~Bundle();
private:
    std::shared_ptr<Components> _components;
    std::unique_ptr<std::byte[]> _ownedData;
    std::byte* _data;
    std::size_t _count;
};
