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
    explicit owned_ptr(T &&object) : _data{std::make_unique<Data>(std::move(object))} {
    }

    ~owned_ptr() {
        assert(_data->_ref_count == 0);
    }

    operator T*() {
        return &_data->_object;
    }

private:
    struct Data {
        size_t _ref_count{};
        T _object;

        explicit Data(T &&object) : _ref_count{0}, _object(std::move(object)) {}
    };

    std::unique_ptr<Data> _data;

    friend class dep_ptr<T>;
};

template<typename T>
class dep_ptr
{
public:
    explicit dep_ptr(owned_ptr<T>& owned) : _owned{owned} {
        _owned._data->_ref_count++;
    }

    ~dep_ptr() {
        _owned._data->_ref_count--;
    }

    operator T*() {
        return &_owned._data->_object;
    }

private:
    owned_ptr<T>& _owned;
};

#endif //OWNED_PTR_OWNED_PTR_H
