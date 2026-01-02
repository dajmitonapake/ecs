#pragma once

#include <cassert>
#include <cstddef>
#include <print>
#include <utility>

template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

/// Waiting for c++ to provide std::is_trivially_relocatable_v
template<typename T>
struct IsTriviallyRelocatable : std::is_trivially_copyable<T> {};

template<typename T>
concept TriviallyRelocatable = IsTriviallyRelocatable<T>::value;

struct TypeInfo {
    bool trivially_relocatable;

    std::size_t size;
    std::size_t align;

    void (*destructor)(std::byte* ptr);
    void (*move_construct)(std::byte* dest, std::byte* src);
    void (*swap)(std::byte* lhs, std::byte* rhs);

    template<typename T>
    static constexpr TypeInfo Of() {
        return TypeInfo{
            .trivially_relocatable = TriviallyRelocatable<T>,
            .size = sizeof(T),
            .align = alignof(T),
            .destructor = [](std::byte* ptr) {
                reinterpret_cast<T*>(ptr)->~T();
            },
            .move_construct = [](std::byte* dest, std::byte* src) {
                new(dest) T(std::move(*reinterpret_cast<T*>(src)));
            },
            .swap = [](std::byte* lhs, std::byte* rhs) {
                std::swap(*reinterpret_cast<T*>(lhs), *reinterpret_cast<T*>(rhs));
            }
        };
    }
};


class BlobVector {
public:
    BlobVector(TypeInfo typeInfo);

    /// Creates a new BlobVector for the given type. Doesn't allocate new memory.
    template<typename T>
    [[nodiscard]] static BlobVector create() {
        auto typeInfo = TypeInfo::Of<T>();
        return BlobVector(typeInfo);
    }

    /// Resizes the vector to the given capacity, allocating new memory if necessary.
    void resize(std::size_t new_capacity);

    template<typename T, typename... Args>
    void emplace(Args&&... args) {
        assert(this->validate<T>());

        if (this->_length == this->_capacity) {
            resize(this->_capacity == 0 ? 4 : this->_capacity * 2);
        }

        auto address = this->data() + this->_length * this->_type_info.size;
        new(address) T(std::forward<Args>(args)...);
        this->_length++;
    }

    /// Grows the vector by the given length, allocating new memory if necessary.
    void grow(std::size_t length);

    /// Pushes element's bytes into back of the vector, while allocating new memory if necessary.
    /// Bytes are copied into the vector and thus the object should not be used unless it is
    /// trivially copyable.
    void push(std::byte* bytes);

    /// Sets element's bytes at the given index. Doesn't call the destructor of the old element
    /// because it should be called on uninitialized memory.
    void set(std::size_t index, std::byte* bytes);

    /// Sets element at the given index. Doesn't call the destructor of the old element
    /// because it should be called on uninitialized memory.
    template<typename T, typename... Args>
    void set(std::size_t index, Args&&... args) {
        assert(this->validate<T>());
        assert(index < this->length());

        auto address = this->get(index);
        new(address) T(std::forward<Args>(args)...);
    }

    /// Replaces old memory with the given bytes at the given index and calls the destructor of
    /// the old element. Bytes are copied into the vector.
    void replace(std::size_t index, std::byte* bytes);

    template<typename T>
    void replace(std::size_t index, T&& value) {
        assert(this->validate<T>());

        if constexpr (TriviallyCopyable<T>) {
            this->replace(index, reinterpret_cast<std::byte*>(value));
        } else {
            T* item = this->get<T>(index);
            *item = std::forward<T>(value);
        }
    }

    /// Swaps the elements at the given indices.
    void swap(std::size_t a, std::size_t b);

    /// Pops the last element from the vector and returns its bytes. Doesn't call the destructor of the
    /// popped element. The pointer gets invalidated after mutation of the vector. This method is
    /// equivalent to decrementing the length of the vector and returning the pointer to the last element.
    [[nodiscard]] std::byte* pop();

    /// Pops the last element and returns a pointer to it. The pointer gets invalidated after mutation of the vector.
    template<typename T>
    [[nodiscard]] T* pop() {
        assert(this->validate<T>());

        return reinterpret_cast<T*>(pop());
    }

    /// Removes the element at the given index and returns pointer to its bytes, filling the gap with the
    /// last element without calling the destructor of the removed element. This method is equivalent to
    /// swapping the given index with the last element and then popping the last element. The pointer gets
    /// invalidated after mutation of the vector.
    [[nodiscard]] std::byte* swapRemove(std::size_t index);

    template<typename T>
    void swapRemove(std::size_t index) {
        assert(this->validate<T>());

        reinterpret_cast<T*>(swapRemove(index))->~T();
    }

    [[nodiscard]] std::byte* get(std::size_t index);

    template<typename T>
    [[nodiscard]] T* get(std::size_t index) {
        assert(this->validate<T>());

        return reinterpret_cast<T*>(get(index));
    }

    [[nodiscard]] std::byte* last();

    template<typename T>
    [[nodiscard]] T* last() {
        assert(this->validate<T>());

        return reinterpret_cast<T*>(last());
    }

    template<typename T>
    [[nodiscard]] T* data() const {
        assert(this->validate<T>());

        return reinterpret_cast<T*>(data());
    }

    [[nodiscard]] std::byte* data() const;
    [[nodiscard]] std::size_t capacity() const;
    [[nodiscard]] std::size_t length() const;
    [[nodiscard]] TypeInfo typeInfo() const;

    template<typename T>
    [[nodiscard]] bool validate() const {
        return this->_type_info.size == sizeof(T) && this->_type_info.align == alignof(T);
    }

    // Copying is not allowed
    BlobVector(const BlobVector&) = delete;
    BlobVector& operator=(const BlobVector&) = delete;

    BlobVector(BlobVector&& other) noexcept
    {
        this->_ptr = other._ptr;
        this->_capacity = other._capacity;
        this->_length = other._length;
        this->_type_info = other._type_info;

        other._ptr = nullptr;
        other._capacity = 0;
        other._length = 0;
    }

    BlobVector& operator=(BlobVector&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        this->~BlobVector();

        this->_ptr = other._ptr;
        this->_capacity = other._capacity;
        this->_length = other._length;
        this->_type_info = other._type_info;

        other._ptr = nullptr;
        other._capacity = 0;
        other._length = 0;

        return *this;
    }

    ~BlobVector();

private:
    std::byte* _ptr;
    std::size_t _capacity;
    std::size_t _length;

    TypeInfo _type_info;
};
