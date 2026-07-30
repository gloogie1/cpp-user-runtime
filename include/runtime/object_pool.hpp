#pragma once

#include "runtime/arena.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace runtime {

template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(std::size_t capacity_bytes);

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ~ObjectPool();

    template <typename... Args>
    T* create(Args&&... args);

    void reset() noexcept;

    std::size_t size() const noexcept;
    std::size_t used_bytes() const noexcept;
    std::size_t capacity_bytes() const noexcept;

private:
    Arena arena_;
    std::vector<T*> live_objects_;

};

template <typename T>
ObjectPool<T>::ObjectPool(std::size_t capacity_bytes)
    : arena_(capacity_bytes) {
}

template <typename T>
ObjectPool<T>::~ObjectPool() {
    reset();
}

template <typename T>
template <typename... Args>
T* ObjectPool<T>::create(Args&&... args) {
    if (live_objects_.size() == live_objects_.capacity()) {
        const std::size_t current_capacity = live_objects_.capacity();
        const std::size_t new_capacity =
            current_capacity == 0 ? 1 : current_capacity * 2;

        live_objects_.reserve(new_capacity);
    }

    const std::size_t arena_mark= arena_.used();
    
    void* memory = arena_.allocate(sizeof(T), alignof(T));

    T* object_memory = static_cast<T*>(memory);
    T* object = nullptr;

    try {
        object = std::construct_at(
            object_memory,
            std::forward<Args>(args)...
        );
    } catch (...){
        arena_.rewind(arena_mark);
        throw;
    }
    live_objects_.push_back(object);
    return object;
}

template <typename T>
void ObjectPool<T>::reset() noexcept {
    for (auto it = live_objects_.rbegin(); it != live_objects_.rend(); ++it){
        std::destroy_at(*it);
    }
    live_objects_.clear();
    arena_.reset();
}

template <typename T>
std::size_t ObjectPool<T>::size() const noexcept {
    return live_objects_.size();
}

template <typename T>
std::size_t ObjectPool<T>::used_bytes() const noexcept {
    return arena_.used();
}

template <typename T>
std::size_t ObjectPool<T>::capacity_bytes() const noexcept {
    return arena_.capacity();
}

}//namespace runtime