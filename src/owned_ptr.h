//
// Created by Helge Penne on 01/11/2023.
//

#ifndef OWNED_PTR_OWNED_PTR_H
#define OWNED_PTR_OWNED_PTR_H

#include <cassert>
#include <memory>

template<typename T>
class dep_ptr;

template<typename T>
class owned_ptr {
public:
    // ToDo: A get_dep could be useful, especially if error handling strategies are introduced

    explicit owned_ptr(T &&object) : _block{Ptr{new Block{0, T{std::move(object)}}, &owned_ptr<T>::deleter}} {
    }

    operator T*() { // NOLINT
        return &_block->object;
    }

    template<class... Args>
    static inline auto make(Args&&... args)
    {
        auto p = new Block{0, T(std::forward<Args>(args)...)};
        return owned_ptr(Ptr{p, &owned_ptr<T>::deleter});
    }

private:
    struct Block {
        size_t ref_count{0};
        T object;

        ~Block() {
            assert(ref_count == 0);
        }
    };

    using Deleter = void(*)(Block*);
    using Ptr = std::unique_ptr<Block, Deleter>;

    Ptr _block;

    explicit owned_ptr(Ptr&& block) : _block{std::move(block)} {}

    static void deleter(Block* b) {
        b->~Block();
    }

    friend class dep_ptr<T>;
};

template<class T, class... Args>
inline auto make_owned(Args&&... args)
{
    return owned_ptr<T>::make(std::forward<Args>(args)...);
}

template<typename T>
class dep_ptr
{
public:
    explicit dep_ptr(owned_ptr<T>& owned) : _block{*owned._block.get()} {
        _block.ref_count++;
    }

    dep_ptr(const dep_ptr& other) : _block{other._block} {
        _block.ref_count++;
    }

    // ToDo: Consider setting _block to nullptr on moves

    dep_ptr(dep_ptr&& other) noexcept : _block{other._block} {
        _block.ref_count++;
    }

    dep_ptr& operator=(const dep_ptr& other) {
        _block.ref_count--;
        _block = other;
        _block.ref_count++;
    }

    dep_ptr& operator=(dep_ptr&& other) noexcept {
        _block.ref_count--;
        _block = other;
        _block.ref_count++;
    }

    ~dep_ptr() {
        _block.ref_count--;
    }

    operator T*() { // NOLINT
        return &_block.object;
    }

private:
    typename owned_ptr<T>::Block& _block;
};

#endif //OWNED_PTR_OWNED_PTR_H
