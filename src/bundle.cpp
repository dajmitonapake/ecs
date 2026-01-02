#include "bundle.hpp"
#include "components.hpp"

#include <cstddef>
#include <memory>

Bundle::Bundle(component_id bitmask) {
    this->bitmask = bitmask;
}

Bundle::Bundle(std::shared_ptr<Components> components, component_id bitmask, std::byte* data, std::size_t count, bool owned) {
    if (owned) {
        std::unique_ptr<std::byte[]> ownedData(data);

        this->_data = nullptr;
        this->_ownedData = std::move(ownedData);
    } else {
        this->_data = data;
        this->_ownedData = nullptr;
    }

    this->_components = components;
    this->bitmask = bitmask;
    this->_count = count;
}

void Bundle::transfer(std::function<void(component_id, std::byte*)> dest) {
    if (this->_count == 0 || (this->_data == nullptr && this->_ownedData == nullptr))
        return;

    std::byte* data = (this->_ownedData) ? this->_ownedData.get() : this->_data;
    auto mask = this->bitmask;

    for (std::size_t i = 0; i < this->_count; ++i) {
        if (mask == 0) break;

        auto index = std::countr_zero(mask);
        auto bit = component_id(1) << index;
        auto info = this->_components->getTypeInfo(bit);

        dest(bit, data);

        data += info.size;

        mask ^= bit;
    }
}


Bundle::~Bundle() {
    if (!this->_ownedData) {
        return;
    }

    auto mask = this->bitmask;
    std::byte* data = (this->_ownedData) ? this->_ownedData.get() : this->_data;

    for (std::size_t i = 0; i < this->_count; ++i) {
        if (mask == 0) break;

        auto index = std::countr_zero(mask);
        auto bit = component_id(1) << index;
        auto info = this->_components->getTypeInfo(bit);

        info.destructor(data);

        data += info.size;

        mask ^= bit;
    }
}
