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
    explicit owned_ptr(Args &&... args) : _storage{new char[block_size()]} {
        new(_storage) Control{owner_marker, &owned_ptr<T, ErrorHandler>::deleter};
        new(_storage + sizeof(Control)) T{std::forward<Args>(args)...};
    }

    /// Creates a new handle and owned object, by copying an existing object of the target type.
    /// \param object The object to copy.
    explicit owned_ptr(const T &object) : _storage{new char[block_size()]} {
        new(_storage) Control{owner_marker, &owned_ptr<T, ErrorHandler>::deleter};
        new(_storage + sizeof(Control)) T{object};
    }

    /// Creates a new handle and owned object, by moving an existing object of the target type.
    /// \param object The object to move from.
    explicit owned_ptr(T &&object) : _storage{new char[block_size()]} {
        new(_storage) Control{owner_marker, &owned_ptr<T, ErrorHandler>::deleter};
        new(_storage + sizeof(Control)) T{std::move(object)};
    }

    /// Copy constructor (deleted)
    owned_ptr(const owned_ptr &other) = delete;

    /// Copy assignment operator (deleted)
    owned_ptr &operator=(const owned_ptr &other) = delete;

    /// Move constructor
    owned_ptr(owned_ptr &&other) noexcept: _storage(other._storage) {
        other._storage = nullptr;
    }

    /// Move assignment
    owned_ptr &operator=(owned_ptr &&other) {
        swap(*this, other);
        return *this;
    }

    /// Destructor.
    /// The owned object is destroyed, but the _storage block on the heap that contains
    /// the reference count, deleter function and the object's memory is retained
    /// until the last dependency is destroyed.
    ~owned_ptr() {
        if (_storage) {
            ref_count() = ref_count() & ~owner_marker;
            get_deleter(_storage)(_storage); // ToDo: Unit tests
            if (!ref_count()) {
                delete_block(_storage);
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
        ErrorHandler::check_condition(_storage, "owned_ptr has been moved from");
        return &get_target(_storage);
    }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_storage, "owned_ptr has been moved from");
        return &get_target(_storage);
    }

    T *operator->() { // NOLINT
        ErrorHandler::check_condition(_storage, "owned_ptr has been moved from");
        return &get_target(_storage);
    }

    const T *operator->() const { // NOLINT
        ErrorHandler::check_condition(_storage, "owned_ptr has been moved from");
        return &get_target(_storage);
    }

    [[nodiscard]] size_t num_deps() const { return ref_count() & ~owner_marker; }

private:
    using Deleter = void (*)(char *);

    struct Control {
        size_t ref_count{};
        Deleter deleter; //NOLINT

        bool has_owner() {
            return ref_count >= owner_marker;
        }
    };

    /// This is a bit mask for the most significant bit of the reference count.
    /// It is set when the owned_ptr handle exists.
    static constexpr size_t owner_marker{1ull << (sizeof(size_t) * 8u - 1u)};

    char *_storage;

    explicit owned_ptr(char *storage) : _storage{storage} {}

    static void deleter(char *storage) {
        get_target(storage).~T();
    }

    static Control &get_control(char *storage) {
        return *reinterpret_cast<Control *>(storage);
    }

    static T &get_target(char *storage) {
        return *reinterpret_cast<T *>(storage + sizeof(Control));
    }

    static Deleter get_deleter(char *storage) {
        return *get_control(storage).deleter;
    }

    static void delete_block(char *storage) {
        get_control(storage).~Control();
        delete[] storage;
    }

    static void swap(owned_ptr &lhs, owned_ptr &rhs) {
        std::swap(lhs._storage, rhs._storage);
    }

    static size_t block_size() { return sizeof(Control) + sizeof(T); }

    size_t &ref_count() const {
        return *reinterpret_cast<size_t *>(_storage);
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
private:
    using Owner = owned_ptr<T, ErrorHandler>;

public:
    explicit dep_ptr(Owner &owned) : _storage{
            owned._storage} {
        ErrorHandler::check_condition(_storage, "owned_ptr has been moved from");
        Owner::get_control(_storage).ref_count++;
    }

    dep_ptr(const dep_ptr &other) : _storage{other._storage} {
        Owner::get_control(_storage).ref_count++;
    }

    dep_ptr &operator=(const dep_ptr &other) {
        dep_ptr tmp(other);
        swap(*this, tmp);
        return *this;
    }

    dep_ptr(dep_ptr &&other) noexcept: _storage{other._storage} {
        other._storage = nullptr;
    }

    dep_ptr &operator=(dep_ptr &&other) noexcept {
        swap(*this, other);
        return *this;
    }

    ~dep_ptr() {
        if (!_storage) {
            return;
        }
        Owner::get_control(_storage).ref_count--;
        if (!Owner::get_control(_storage).ref_count) {
            Owner::delete_block(reinterpret_cast<char *>(_storage));
        }
    }

    operator T *() { // NOLINT
        ErrorHandler::check_condition(_storage, "dep_ptr has been moved from");
        ErrorHandler::check_condition(Owner::get_control(_storage).has_owner(), "owner has been deleted");
        return &Owner::get_target(_storage);
    }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_storage, "dep_ptr has been moved from");
        ErrorHandler::check_condition(Owner::get_control(_storage).has_owner(), "owner has been deleted");
        return &Owner::get_target(_storage);
    }

    T *operator->() { // NOLINT
        ErrorHandler::check_condition(_storage, "dep_ptr has been moved from");
        ErrorHandler::check_condition(Owner::get_control(_storage).has_owner(), "owner has been deleted");
        return &Owner::get_target(_storage);
    }

    const T *operator->() const { // NOLINT
        ErrorHandler::check_condition(_storage, "dep_ptr has been moved from");
        ErrorHandler::check_condition(Owner::get_control(_storage).has_owner(), "owner has been deleted");
        return &Owner::get_target(_storage);
    }

private:
    char *_storage;

    static void swap(dep_ptr &lhs, dep_ptr &rhs) {
        std::swap(lhs._storage, rhs._storage);
    }
};

template<typename T, class ErrorHandler>
class dep_ptr_const {
private:
    using Owner = owned_ptr<T, ErrorHandler>;

public:
    explicit dep_ptr_const(const Owner &owned) : _storage{owned._storage} {
        ErrorHandler::check_condition(_storage, "owned_ptr has been moved from");
        Owner::get_control(_storage).ref_count++;
    }

    dep_ptr_const(const dep_ptr_const &other) : _storage{other._storage} {
        Owner::get_control(_storage).ref_count++;
    }

    dep_ptr_const &operator=(const dep_ptr_const &other) {
        dep_ptr_const tmp(other);
        swap(*this, tmp);
        return *this;
    }

    dep_ptr_const(dep_ptr_const &&other) noexcept: _storage{other._storage} {
        other._storage = nullptr;
    }

    dep_ptr_const &operator=(dep_ptr_const &&other) noexcept {
        swap(*this, other);
        return *this;
    }

    ~dep_ptr_const() {
        if (!_storage) {
            return;
        }
        Owner::get_control(_storage).ref_count--;
        if (!Owner::get_control(_storage).ref_count) {
            Owner::delete_block(reinterpret_cast<char *>(_storage));
        }
    }

    operator const T *() const { // NOLINT
        ErrorHandler::check_condition(_storage, "dep_ptr has been moved from");
        ErrorHandler::check_condition(Owner::get_control(_storage).has_owner(), "owner has been deleted");
        return &Owner::get_target(_storage);
    }

    const T *operator->() const { // NOLINT
        ErrorHandler::check_condition(_storage, "dep_ptr has been moved from");
        ErrorHandler::check_condition(Owner::get_control(_storage).has_owner(), "owner has been deleted");
        return &Owner::get_target(_storage);
    }

private:
    char *_storage;

    static void swap(dep_ptr_const &lhs, dep_ptr_const &rhs) {
        std::swap(lhs._storage, rhs._storage);
    }
};

#endif //OWNED_PTR_OWNED_PTR_H
