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
    /// Creates a new handle and owned object, by moving an existing object of the target type.
    /// Takes the same parameters as the target type's constructor, moves the arguments,
    /// and constructs the target object in-place.
    template<class... Args>
    explicit owned_ptr(Args &&... args) : _block{new char[block_size()]} {
        new(_block) Block{owner_marker, &owned_ptr<T, ErrorHandler>::deleter, T{std::forward<Args>(args)...}};
    }

    /// Creates a new handle and owned object, by copying an existing object of the target type.
    /// \param object The object to copy.
    explicit owned_ptr(const T &object) : _block{new char[block_size()]} {
        new(_block) Block{owner_marker, &owned_ptr<T, ErrorHandler>::deleter, T{object}};
    }

    /// Creates a new handle and owned object, by moving an existing object of the target type.
    /// \param object The object to move from.
    explicit owned_ptr(T &&object) : _block{new char[block_size()]} {
        new(_block) Block{owner_marker, &owned_ptr<T, ErrorHandler>::deleter, T{std::move(object)}};
    }

    /// Copy constructor (deleted)
    owned_ptr(const owned_ptr& other) = delete;

    /// Copy assignment operator (deleted)
    owned_ptr& operator=(const owned_ptr& other) = delete;

    /// Move constructor
    owned_ptr(owned_ptr&& other) noexcept : _block(other._block) {
        other._block = nullptr;
    }

    /// Move assignment
    owned_ptr& operator=(owned_ptr&& other){
        swap(*this, other);
        return *this;
    }

    /// Destructor.
    /// The owned object is destroyed, but the "block" on the heap that contains
    /// the reference count, deleter function and the object's memory is retained
    /// until the last dependency is destroyed.
    ~owned_ptr() {
        if (_block) {
            ref_count() = ref_count() & ~owner_marker;
            get_deleter(_block)(_block); // ToDo: Unit tests
            if (!ref_count()) {
                delete_block(_block);
                _block = nullptr;
            }
        }
    }

    auto make_dep() {
        return dep_ptr<T, ErrorHandler>{*this};
    }

    auto make_dep() const {
        return dep_ptr_const<T, ErrorHandler>{*this};
    }

    operator T *() { // NOLINT
        ErrorHandler::check_condition(_block, "owned_ptr has been moved from");
        return &reinterpret_cast<Block *>(_block)->object;
    }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_block, "owned_ptr has been moved from");
        return &reinterpret_cast<Block *>(_block)->object;
    }

    T *operator->() { // NOLINT
        ErrorHandler::check_condition(_block, "owned_ptr has been moved from");
        return &reinterpret_cast<Block *>(_block)->object;
    }

    const T *operator->() const { // NOLINT
        ErrorHandler::check_condition(_block, "owned_ptr has been moved from");
        return &reinterpret_cast<Block *>(_block)->object;
    }

    [[nodiscard]] size_t num_deps() const { return ref_count() & ~owner_marker; }

private:
    using Deleter = void (*)(char *);

    struct Block {
        size_t ref_count{};
        Deleter deleter; //NOLINT
        T object;

        bool has_owner() {
            return ref_count >= owner_marker;
        }
    };

    /// This is a bit mask for the most significant bit of the reference count.
    /// It is set when the owned_ptr handle exists.
    static constexpr size_t owner_marker{1ull << (sizeof(size_t) * 8u - 1u)};

    char *_block;

    explicit owned_ptr(char *block) : _block{block} {}

    static void deleter(char *block) {
        // ToDo: Consider if we need to destroy only the T here
        reinterpret_cast<Block *>(block)->~Block();
    }

    static Deleter get_deleter(char* block) {
        return *reinterpret_cast<Deleter *>(block + sizeof(size_t));
    }

    static void delete_block(char* block) {
        delete[] block;
    }

    static void swap(owned_ptr &lhs, owned_ptr &rhs) {
        std::swap(lhs._block, rhs._block);
    }

    static size_t block_size() { return sizeof(Block); }

    size_t &ref_count() const {
        return *reinterpret_cast<size_t *>(_block);
    };

    friend class dep_ptr<T, ErrorHandler>;

    friend class dep_ptr_const<T, ErrorHandler>;
};

template<class T, class... Args>
inline auto make_owned(Args &&... args) {
    return owned_ptr<T, owned_ptr_error_handler>(std::forward<Args>(args)...);
}

template<typename T, class ErrorHandler>
class dep_ptr {
public:
    explicit dep_ptr(owned_ptr<T, ErrorHandler> &owned) : _block{
            reinterpret_cast<typename owned_ptr<T, ErrorHandler>::Block *>(owned._block)} {
        ErrorHandler::check_condition(_block, "owned_ptr has been moved from");
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
        swap(*this, other);
        return *this;
    }

    ~dep_ptr() {
        if (!_block) {
            return;
        }
        _block->ref_count--;
        if (!_block->ref_count) {
            owned_ptr<T, ErrorHandler>::delete_block(reinterpret_cast<char*>(_block));
            _block = nullptr;
        }
    }

    operator T *() { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        ErrorHandler::check_condition(_block->has_owner(), "owner has been deleted");
        return &_block->object;
    }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        ErrorHandler::check_condition(_block->has_owner(), "owner has been deleted");
        return &_block->object;
    }

    T *operator->() { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        ErrorHandler::check_condition(_block->has_owner(), "owner has been deleted");
        return &_block->object;
    }

    const T *operator->() const { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        ErrorHandler::check_condition(_block->has_owner(), "owner has been deleted");
        return &_block->object;
    }

private:
    typename owned_ptr<T, ErrorHandler>::Block *_block;

    static void swap(dep_ptr &lhs, dep_ptr &rhs) {
        std::swap(lhs._block, rhs._block);
    }
};

template<typename T, class ErrorHandler>
class dep_ptr_const {
public:
    explicit dep_ptr_const(const owned_ptr<T, ErrorHandler> &owned) : _block{
            reinterpret_cast<typename owned_ptr<T, ErrorHandler>::Block *>(owned._block)} {
        ErrorHandler::check_condition(_block, "owned_ptr has been moved from");
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
        swap(*this, other);
        return *this;
    }

    ~dep_ptr_const() {
        if (!_block) {
            return;
        }
        _block->ref_count--;
        if (!_block->ref_count) {
            owned_ptr<T, ErrorHandler>::delete_block(reinterpret_cast<char*>(_block));
            _block = nullptr;
        }
    }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        ErrorHandler::check_condition(_block->has_owner(), "owner has been deleted");
        return &_block->object;
    }

    const T *operator->() const { // NOLINT
        ErrorHandler::check_condition(_block, "dep_ptr has been moved from");
        ErrorHandler::check_condition(_block->has_owner(), "owner has been deleted");
        return &_block->object;
    }

private:
    typename owned_ptr<T, ErrorHandler>::Block *_block;

    static void swap(dep_ptr_const &lhs, dep_ptr_const &rhs) {
        std::swap(lhs._block, rhs._block);
    }
};

#endif //OWNED_PTR_OWNED_PTR_H
