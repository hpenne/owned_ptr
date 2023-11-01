//
// Created by Helge Penne on 01/11/2023.
//

#ifndef OWNED_PTR_OWNED_PTR_H
#define OWNED_PTR_OWNED_PTR_H

#include <cassert>
#include <memory>

struct owned_ptr_error_handler {
    static void check_condition(bool condition, const char *reason) {
        (void) reason;
        assert(condition);
    }

    static constexpr bool reset_moved_from_dep_ptr{false};
};

template<typename T, class ErrorHandler>
class dep_ptr;

template<typename T, class ErrorHandler = owned_ptr_error_handler>
class owned_ptr {
public:
    explicit owned_ptr(T &&object) : _block{
            Ptr{new Block{0, T{std::move(object)}}, &owned_ptr<T, ErrorHandler>::deleter}} {
    }

    operator T *() { // NOLINT
        return &_block->object;
    }

    template<class... Args>
    static inline auto make(Args &&... args) {
        auto p = new Block{0, T(std::forward<Args>(args)...)};
        return owned_ptr(Ptr{p, &owned_ptr<T, ErrorHandler>::deleter});
    }

    auto make_dep() {
        return dep_ptr<T, ErrorHandler>{*this};
    }

private:
    struct Block {
        size_t ref_count{0};
        T object;

        ~Block() {
            ErrorHandler::check_condition(ref_count == 0, "One or more remaining dep_ptr when deleting owned_ptr");
            assert(ref_count == 0);
        }
    };

    using Deleter = void (*)(Block *);
    using Ptr = std::unique_ptr<Block, Deleter>;

    Ptr _block;

    explicit owned_ptr(Ptr &&block) : _block{std::move(block)} {}

    static void deleter(Block *b) {
        b->~Block();
    }

    friend class dep_ptr<T, ErrorHandler>;
};

template<class T, class... Args>
inline auto make_owned(Args &&... args) {
    return owned_ptr<T, owned_ptr_error_handler>::make(std::forward<Args>(args)...);
}

template<typename T, class ErrorHandler>
class dep_ptr {
public:
    explicit dep_ptr(owned_ptr<T, ErrorHandler> &owned) : _block{owned._block.get()} {
        _block->ref_count++;
    }

    dep_ptr(const dep_ptr &other) : _block{other._block} {
        _block->ref_count++;
    }

    dep_ptr &operator=(const dep_ptr &other) {
        dep_ptr tmp(other);
        swap(*this, tmp);
        return *this;
    }

    dep_ptr(dep_ptr &&other) noexcept: _block{other._block} {
        if constexpr (ErrorHandler::reset_moved_from_dep_ptr) {
            other._block = nullptr;
        } else {
            _block->ref_count++;
        }
    }

    dep_ptr &operator=(dep_ptr &&other) noexcept {
        dep_ptr tmp(std::move(other));
        swap(*this, tmp);
        return *this;
    }

    ~dep_ptr() {
        if constexpr (ErrorHandler::reset_moved_from_dep_ptr) {
            if (!_block) {
                return;
            }
        }
        _block->ref_count--;
    }

    operator T *() { // NOLINT
        if constexpr (ErrorHandler::reset_moved_from_dep_ptr) {
            ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        }
        return &_block->object;
    }

private:
    typename owned_ptr<T, ErrorHandler>::Block *_block;

    static void swap(dep_ptr& a, dep_ptr& b) {
        std::swap(a._block, b._block);
    }
};

#endif //OWNED_PTR_OWNED_PTR_H
