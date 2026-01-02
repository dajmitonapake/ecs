#include "blob_vector.hpp"

#include <algorithm>

BlobVector::BlobVector(TypeInfo typeInfo) {
    this->_ptr = nullptr;
    this->_capacity = 0;
    this->_length = 0;

    this->_type_info = typeInfo;
}

void BlobVector::resize(std::size_t new_capacity) {
    std::byte* new_ptr = static_cast<std::byte*>(operator new(
        new_capacity * this->_type_info.size,
        std::align_val_t{this->_type_info.align}
    ));

    if (this->_ptr) {
        if (this->_type_info.trivially_relocatable) {
            std::copy(this->_ptr, this->_ptr + this->_length * this->_type_info.size, new_ptr);
            operator delete(this->_ptr, this->_capacity * this->_type_info.size, std::align_val_t{this->_type_info.align});
        } else {
            for (std::size_t i = 0; i < this->_length; ++i) {
                std::byte* old_item = this->_ptr + i * this->_type_info.size;
                std::byte* new_item = new_ptr + i * this->_type_info.size;

                this->_type_info.move_construct(new_item, old_item);
                this->_type_info.destructor(old_item);
            }
            operator delete(this->_ptr, this->_capacity * this->_type_info.size, std::align_val_t{this->_type_info.align});
        }
    }

    this->_ptr = new_ptr;
    this->_capacity = new_capacity;
}

void BlobVector::grow(std::size_t length) {
    if (this->_length == this->_capacity) {
        resize(this->_capacity == 0 ? 4 : this->_capacity * 2);
    }
    this->_length += length;
}

void BlobVector::push(std::byte* bytes) {
    if (this->_length == this->_capacity) {
        resize(this->_capacity == 0 ? 4 : this->_capacity * 2);
    }

    auto address = this->_ptr + this->_length * this->_type_info.size;
    std::copy(bytes, bytes + this->_type_info.size, address);
    this->_length++;
}

void BlobVector::set(std::size_t index, std::byte* bytes) {
    assert(index < this->_length);

    auto address = this->_ptr + index * this->_type_info.size;

    if (this->_type_info.trivially_relocatable) {
        std::copy(bytes, bytes + this->_type_info.size, address);
    } else {
        this->_type_info.move_construct(address, bytes);
    }
}

void BlobVector::replace(std::size_t index, std::byte* bytes) {
    assert(index < this->_length);

    auto old_address = this->get(index);

    this->_type_info.destructor(old_address);

    if (this->_type_info.trivially_relocatable) {
        std::copy(bytes, bytes + this->_type_info.size, old_address);
    } else {
        this->_type_info.move_construct(old_address, bytes);
    }
}

void BlobVector::swap(std::size_t a, std::size_t b) {
    assert(a < this->_length);
    assert(b < this->_length);

    auto a_ptr = this->get(a);
    auto b_ptr = this->get(b);

    if (a == b) return;

    if (this->_type_info.trivially_relocatable) {
        alignas(std::max_align_t) std::byte temp_stack[64];
        std::byte* temp = temp_stack;
        bool allocated = false;

        if (this->_type_info.size > sizeof(temp_stack)) {
            temp = static_cast<std::byte*>(operator new(this->_type_info.size, std::align_val_t{this->_type_info.align}));
            allocated = true;
        }

        std::copy(a_ptr, a_ptr + this->_type_info.size, temp);
        std::copy(b_ptr, b_ptr + this->_type_info.size, a_ptr);
        std::copy(temp, temp + this->_type_info.size, b_ptr);

        if (allocated) {
            operator delete(temp, this->_type_info.size, std::align_val_t{this->_type_info.align});
        }
    } else {
        this->_type_info.swap(a_ptr, b_ptr);
    }
}

std::byte* BlobVector::pop() {
    assert(this->_length > 0);

    this->_length--;
    return this->get(this->_length);
}

std::byte* BlobVector::swapRemove(std::size_t index) {
    assert(index < this->_length);

    this->swap(index, this->_length - 1);
    return this->pop();
}

std::byte* BlobVector::get(std::size_t index) {
    assert(index < this->_length);

    return this->_ptr + index * this->_type_info.size;
}

std::byte* BlobVector::last() {
    assert(this->_length > 0);

    return this->get(this->_length - 1);
}

std::byte* BlobVector::data() const {
    return this->_ptr;
}

std::size_t BlobVector::capacity() const {
    return this->_capacity;
}

std::size_t BlobVector::length() const {
    return this->_length;
}

TypeInfo BlobVector::typeInfo() const {
    return this->_type_info;
}

BlobVector::~BlobVector() {
    if (this->_ptr) {
        // Call destructor for each element
        for (std::size_t i = 0; i < this->_length; ++i) {
            this->_type_info.destructor(this->get(i));
        }
        operator delete(this->_ptr, this->_capacity * this->_type_info.size, std::align_val_t{this->_type_info.align});
    }
}
