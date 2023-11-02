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
};

template<typename T, class ErrorHandler>
class dep_ptr;

template<typename T, class ErrorHandler>
class dep_ptr_const;

template<typename T, class ErrorHandler = owned_ptr_error_handler>
class owned_ptr {
public:
    explicit owned_ptr(T &&object) : _block{
            new Block{owner_marker, &owned_ptr<T, ErrorHandler>::deleter, T{std::move(object)}}} {
    }

    ~owned_ptr() {
        _block->ref_count = _block->ref_count & ~owner_marker;
        if (!_block->ref_count) {
            auto deleter = _block->deleter;
            deleter(_block);
        }
    }

    operator T *() { // NOLINT
        return &_block->object;
    }

    operator const T *() const { // NOLINT
        return &_block->object;
    }

    T *operator->() { // NOLINT
        return &_block->object;
    }

    const T *operator->() const { // NOLINT
        return &_block->object;
    }

    template<class... Args>
    static inline auto make(Args &&... args) {
        return owned_ptr(new Block{owner_marker, &owned_ptr<T, ErrorHandler>::deleter, T(std::forward<Args>(args)...)});
    }

    auto make_dep() {
        return dep_ptr<T, ErrorHandler>{*this};
    }

    auto make_dep() const {
        return dep_ptr_const<T, ErrorHandler>{*this};
    }

    [[nodiscard]] size_t num_deps() const { return _block->ref_count & ~owner_marker; }

private:
    static constexpr size_t owner_marker{1ull << (sizeof(size_t) * 8u - 1u)};
    struct Block;
    using Deleter = void (*)(Block *);

    struct Block {
        size_t ref_count{};
        Deleter deleter;
        T object;

        ~Block() {
            ErrorHandler::check_condition(ref_count == 0, "One or more remaining dep_ptr when deleting owned_ptr");
            assert(ref_count == 0);
        }
    };

    Block* _block;

    explicit owned_ptr(Block* block) : _block{block} {}

    static void deleter(Block *b) {
        b->~Block();
    }

    friend class dep_ptr<T, ErrorHandler>;

    friend class dep_ptr_const<T, ErrorHandler>;
};

template<class T, class... Args>
inline auto make_owned(Args &&... args) {
    return owned_ptr<T, owned_ptr_error_handler>::make(std::forward<Args>(args)...);
}

template<typename T, class ErrorHandler>
class dep_ptr {
public:
    explicit dep_ptr(owned_ptr<T, ErrorHandler> &owned) : _block{owned._block} {
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
        other._block = nullptr;
    }

    dep_ptr &operator=(dep_ptr &&other) noexcept {
        dep_ptr tmp(std::move(other));
        swap(*this, tmp);
        return *this;
    }

    ~dep_ptr() {
        if (!_block) {
            return;
        }
        _block->ref_count--;
        if (!_block->ref_count) {
            auto deleter = _block->deleter;
            deleter(_block);
        }
    }

    operator T *() { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        return &_block->object;
    }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        return &_block->object;
    }

    T *operator->() { // NOLINT
        return &_block->object;
    }

    const T *operator->() const { // NOLINT
        return &_block->object;
    }

private:
    typename owned_ptr<T, ErrorHandler>::Block *_block;

    static void swap(dep_ptr &a, dep_ptr &b) {
        std::swap(a._block, b._block);
    }
};

template<typename T, class ErrorHandler>
class dep_ptr_const {
public:
    explicit dep_ptr_const(const owned_ptr<T, ErrorHandler> &owned) : _block{owned._block} {
        _block->ref_count++;
    }

    dep_ptr_const(const dep_ptr_const &other) : _block{other._block} {
        _block->ref_count++;
    }

    dep_ptr_const &operator=(const dep_ptr_const &other) {
        dep_ptr_const tmp(other);
        swap(*this, tmp);
        return *this;
    }

    dep_ptr_const(dep_ptr_const &&other) noexcept: _block{other._block} {
        other._block = nullptr;
    }

    dep_ptr_const &operator=(dep_ptr_const &&other) noexcept {
        dep_ptr_const tmp(std::move(other));
        swap(*this, tmp);
        return *this;
    }

    ~dep_ptr_const() {
        if (!_block) {
            return;
        }
        _block->ref_count--;
        if (!_block->ref_count) {
            auto deleter = _block->deleter;
            deleter(_block);
        }
   }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr_const has been moved from");
        return &_block->object;
    }

    const T *operator->() const { // NOLINT
        return &_block->object;
    }

private:
    typename owned_ptr<T, ErrorHandler>::Block *_block;

    static void swap(dep_ptr_const &a, dep_ptr_const &b) {
        std::swap(a._block, b._block);
    }
};

#endif //OWNED_PTR_OWNED_PTR_H
