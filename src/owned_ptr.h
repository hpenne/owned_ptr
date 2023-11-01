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
    explicit owned_ptr(T &&object) : _object{Ptr(new T{std::move(object)}, &owned_ptr<T>::delete_t)} {
    }

    ~owned_ptr() {
        assert(_ref_count == 0);
    }

    operator T*() {
        return _object.get();
    }

private:
    using Deleter = void(*)(T*);
    using Ptr = std::unique_ptr<T, Deleter>;

    size_t _ref_count{0};
    Ptr _object;

    static void delete_t(T* t) {
        t->~T();
    }

    friend class dep_ptr<T>;
};

template<typename T>
class dep_ptr
{
public:
    explicit dep_ptr(owned_ptr<T>& owned) : _owned{owned}, _object{owned._object.get()} {
        _owned._ref_count++;
    }

    ~dep_ptr() {
        _owned._ref_count--;
    }

    operator T*() {
        return _object;
    }

private:
    owned_ptr<T>& _owned;
    T* _object;
};

#endif //OWNED_PTR_OWNED_PTR_H
