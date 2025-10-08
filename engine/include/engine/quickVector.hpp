#pragma once

#include "alloc.hpp"
#include "engine/errorHandling.hpp"
#include "template.hpp"
#include <algorithm>
#include <tchar.h>
#include <type_traits>

namespace
{
inline constexpr bool isDebugMode()
{
#ifdef DEBUG
    return true;
#endif
    return false;
}
} // namespace

namespace engine
{
// A simple vector implementation like std::vector but without iterators and without default initialization of
// data
template <typename T, float Increment = 2.f, typename Alloc = alloc<T>, bool DebugChecks = isDebugMode()>
struct quickVector
{
    static_assert(Increment > 1, "Increment must be greater than 1");

    template <typename, float, typename, bool>
    friend struct quickVector;

    quickVector() = default;

    // initialize vector with initial capacity (length of internal array)
    quickVector(const size_t initialCapacity)
        : _size(0), _capacity(initialCapacity)
    {
        _data = Alloc::allocate(_capacity);
    }

    // initialize vector with initial values
    template <typename... TObjects>
        requires std::conjunction_v<std::is_same<TObjects, T>...>
    quickVector(bool, TObjects &&...initObjects)
        : _capacity(sizeof...(TObjects)), _size(sizeof...(TObjects))
    {
        _data = Alloc::allocate(_capacity);
        setVariadic_(0, std::forward<TObjects>(initObjects)...);
    }

    // copy reference (basic form | otherwise it won't compile)
    quickVector(const quickVector &other)
        : _size(other._size), _capacity(other._capacity)
    {
        _data = Alloc::allocate(_capacity);
        for (size_t i = 0; i < _size; i++)
            new (&_data[i]) T(other._data[i]);
    }

    // copy reference
    template <typename OtherT, float OtherIncrement>
        requires std::is_convertible_v<OtherT, T>
    quickVector(const quickVector<OtherT, OtherIncrement> &other)
        : _size(other._size), _capacity(other._capacity)
    {
        _data = Alloc::allocate(_capacity);
        for (size_t i = 0; i < _size; i++)
            new (&_data[i]) T(other._data[i]);
    }

    // move reference (basic form | otherwise it won't compile)
    quickVector(quickVector &&other) noexcept
        : _data(std::exchange(other._data, nullptr)),
          _size(std::exchange(other._size, 0)),
          _capacity(std::exchange(other._capacity, 0))
    {
    }

    // move reference
    template <float OtherIncrement>
    quickVector(quickVector<T, OtherIncrement> &&other) noexcept
        : _data(std::exchange(other._data, nullptr)),
          _size(std::exchange(other._size, 0)),
          _capacity(std::exchange(other._capacity, 0))
    {
    }

    // copy reference (basic form | otherwise it won't compile)
    quickVector &operator=(const quickVector &other)
    {
        assertDuringForEach();
        if (!equals_(this, &other))
        {
            realloc_(other._capacity);
            _size = other._size;
            for (size_t i = 0; i < _size; i++)
                new (&_data[i]) T(other._data[i]);
        }
        return *this;
    }

    // copy reference
    template <float OtherIncrement>
    quickVector &operator=(const quickVector<T, OtherIncrement> &other)
    {
        assertDuringForEach();
        if (!equals_(this, &other))
        {
            realloc_(other._capacity);
            _size = other._size;
            for (size_t i = 0; i < _size; i++)
                new (&_data[i]) T(other._data[i]);
        }
        return *this;
    }

    // move reference (basic form | otherwise it won't compile)
    quickVector &operator=(quickVector &&other) noexcept
    {
        assertDuringForEach();
        if (!equals_(this, &other))
        {
            Alloc::deallocate(_data, _capacity);
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0);
            _capacity = std::exchange(other._capacity, 0);
        }
        return *this;
    }

    // move reference
    template <float OtherIncrement>
    quickVector &operator=(quickVector<T, OtherIncrement> &&other) noexcept
    {
        assertDuringForEach();
        if (!equals_(this, &other))
        {
            Alloc::deallocate(_data, _capacity);
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0);
            _capacity = std::exchange(other._capacity, 0);
        }
        return *this;
    }

    ~quickVector()
    {
        if (_data)
        {
            forEach([](T &item) {
                destructItem_(item);
            });
            Alloc::deallocate(_data, _capacity);
        }
    }

    // equality check by reference (basic form | otherwise it won't compile)
    bool operator==(const quickVector &other) const
    {
        return equals_(this, &other);
    }

    // equality check by reference
    template <typename OtherT, float OtherIncrement, typename OtherAlloc, bool OtherDebugChecks>
    bool operator==(const quickVector<OtherT, OtherIncrement, OtherAlloc, OtherDebugChecks> &other) const
    {
        return equals_(this, &other);
    }

    // inequality check by reference (basic form | otherwise it won't compile)
    bool operator!=(const quickVector &other) const
    {
        return !equals_(this, &other);
    }

    // inequality check by reference
    template <typename OtherT, float OtherIncrement, typename OtherAlloc, bool OtherDebugChecks>
    bool operator!=(const quickVector<OtherT, OtherIncrement, OtherAlloc, OtherDebugChecks> &other) const
    {
        return !equals_(this, &other);
    }

    void push_back(const T &value)
    {
        assertDuringForEach();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        new (&_data[_size++]) T(value);
    }

    void push_back(const T &&value)
    {
        assertDuringForEach();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        new (&_data[_size++]) T(std::move(value));
    }

    template <typename... Args>
    void emplace_back(Args &&...args)
    {
        assertDuringForEach();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        new (&_data[_size++]) T(std::forward<Args>(args)...);
    }

    void pop_back()
    {
        assertDuringForEach();
        assertRange_(0);
        destructItem_(_data[_size - 1]);
        _size--;
    }

    void reserve(const size_t capacity)
    {
        assertDuringForEach();
        if (capacity > _capacity)
            realloc_(capacity);
    }

    template <typename Value>
    void insert(const size_t index, Value &&item)
    {
        assertDuringForEach();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        _size++;
        assertRange_(index);
        for (size_t i = _size; i-- > index + 1;)
            new (&_data[i]) T(std::move(_data[i - 1]));
        new (&_data[index]) T(std::move(item));
    }

    template <typename... Args>
    void emplace(const size_t index, Args &&...args)
    {
        assertDuringForEach();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        _size++;
        assertRange_(index);
        for (size_t i = _size; i-- > index + 1;)
            new (&_data[i]) T(std::move(_data[i - 1]));
        new (&_data[index]) T(std::forward<Args>(args)...);
    }

    void clear()
    {
        assertDuringForEach();
        forEach([](T &item) {
            destructItem_(item);
        });
        _size = 0;
    }

    template <typename Func>
    void forEachAndClear(Func &&func)
    {
        assertDuringForEach();
        forEach([&func](T &item) {
            func(item);
            destructItem_(item);
        });
        _size = 0;
    }

    // will be invalid if there's no items
    T &back()
    {
        return _data[_size - 1];
    }

    // will be invalid if there's no items
    const T &back() const
    {
        return _data[_size - 1];
    }

    template <typename Func>
    void forEach(Func &&func)
    {
        if constexpr (DebugChecks)
            _duringForEach.value = true;
        for (size_t i = 0; i < _size; ++i)
            func(_data[i]);
        if constexpr (DebugChecks)
            _duringForEach.value = false;
    }

    template <typename Func>
    void forEach(Func &&func) const
    {
        if constexpr (DebugChecks)
            const_cast<quickVector *>(this)->_duringForEach.value = true;
        for (size_t i = 0; i < _size; ++i)
            func(static_cast<const T &>(_data[i]));
        if constexpr (DebugChecks)
            const_cast<quickVector *>(this)->_duringForEach.value = false;
    }

    template <typename Func>
    void forEachParallel(Func &&func)
    {
        if constexpr (DebugChecks)
            _duringForEach.value = true;
#pragma omp parallel for
        for (signed long long i = 0; i < _size; ++i)
            func(_data[i]);
        if constexpr (DebugChecks)
            _duringForEach.value = false;
    }

    template <typename Func>
    void forEachParallel(Func &&func) const
    {
        if constexpr (DebugChecks)
            _duringForEach.value = true;
#pragma omp parallel for
        for (signed long long i = 0; i < _size; ++i)
            func(static_cast<const T &>(_data[i]));
        if constexpr (DebugChecks)
            _duringForEach.value = false;
    }

    template <typename Func>
    void forEachIndexed(Func &&func)
    {
        if constexpr (DebugChecks)
            _duringForEach.value = true;
        for (size_t i = 0; i < _size; ++i)
            func(i, _data[i]);
        if constexpr (DebugChecks)
            _duringForEach.value = false;
    }

    template <typename Func>
    void forEachIndexed(Func &&func) const
    {
        if constexpr (DebugChecks)
            _duringForEach.value = true;
        for (size_t i = 0; i < _size; ++i)
            func(i, static_cast<const T &>(_data[i]));
        if constexpr (DebugChecks)
            _duringForEach.value = false;
    }

    template <typename Func>
    void forEachIndexedParallel(Func &&func)
    {
        if constexpr (DebugChecks)
            _duringForEach.value = true;
#pragma omp parallel for
        for (signed long long i = 0; i < _size; ++i)
            func(i, _data[i]);
        if constexpr (DebugChecks)
            _duringForEach.value = false;
    }

    template <typename Func>
    void forEachIndexedParallel(Func &&func) const
    {
        if constexpr (DebugChecks)
            const_cast<quickVector *>(this)->_duringForEach.value = true;
#pragma omp parallel for
        for (signed long long i = 0; i < _size; ++i)
            func(i, static_cast<const T &>(_data[i]));
        if constexpr (DebugChecks)
            const_cast<quickVector *>(this)->_duringForEach.value = false;
    }

    void erase(const T &item)
    {
        assertDuringForEach();
        size_t write = 0;
        for (size_t read = 0; read < _size; ++read)
        {
            if (!(_data[read] == item))
            {
                if (write != read)
                    _data[write] = std::move(_data[read]);
                ++write;
            }
            else
            {
                destructItem_(_data[read]);
            }
        }
        _size = write;
    }

    template <typename Func>
    void eraseIf(Func &&func)
    {
        assertDuringForEach();
        size_t write = 0;
        for (size_t read = 0; read < _size; ++read)
        {
            if (!func(_data[read]))
            {
                if (write != read)
                    _data[write] = std::move(_data[read]);
                ++write;
            }
            else
            {
                if constexpr (!std::is_pointer_v<T>)
                    destructItem_(_data[read]);
            }
        }
        _size = write;
    }

    template <typename Func>
    void eraseIfUnordered(Func &&func)
    {
        assertDuringForEach();
        for (size_t i = 0; i < _size;)
        {
            if (func(_data[i]))
            {
                destructItem_(_data[i]);
                _data[i] = std::move(_data[--_size]);
            }
            else
            {
                ++i;
            }
        }
    }

    // finds all items of type
    template <typename OtherT, typename OtherAlloc, bool OtherDebugMode>
    const void findAllByType(quickVector<OtherT> &result) const
    {
        for (size_t i = 0; i < _size; i++)
            if (OtherT *r = dynamic_cast<OtherT *>(_data[i]))
                result.push_back(*r);
    }

    // finds item by condition. returns nullptr if not found
    template <typename Func>
    T *findIf(Func &&func) const
    {
        for (size_t i = 0; i < _size; ++i)
            if (func(static_cast<const T &>(_data[i])))
                return &_data[i];
        return nullptr;
    }

    // finds items by condition
    template <float OtherIncrement, typename OtherAlloc, bool OtherDebugMode, typename Func>
    void findAllIf(quickVector<T, OtherIncrement, OtherAlloc, OtherDebugMode> &result, Func &&func) const
    {
        for (size_t i = 0; i < _size; ++i)
            if (func(_data[i]))
                result.push_back(_data[i]);
    }

    // finds item (returns nullptr if not found)
    T *find(const T &item) const
    {
        for (size_t i = 0; i < _size; ++i)
            if (item == _data[i])
                return &_data[i];
        return nullptr;
    }

    // finds item (returns -1 (size_t's max value) if not found)
    size_t findIndex(const T &item) const
    {
        for (size_t i = 0; i < _size; ++i)
            if (item == _data[i])
                return i;
        return static_cast<size_t>(-1);
    }

    // get count of the vector
    size_t size() const
    {
        return _size;
    }

    // get internal pointer
    T *data() const
    {
        return _data;
    }

    T &operator[](const size_t index)
    {
        assertRange_(index);
        return _data[index];
    }

    const T &operator[](const size_t index) const
    {
        assertRange_(index);
        return _data[index];
    }

  private:
    T *_data = nullptr;
    size_t _size = 0, _capacity = 0;
    ConditionalVariable<bool, false, DebugChecks> _duringForEach;

    template <typename First, typename... Rest>
    void setVariadic_(const size_t index, First &&first, Rest &&...rest)
    {
        new (&_data[index]) T(std::forward<First>(first));
        setVariadic_(index + 1, std::forward<Rest>(rest)...);
    }
    template <typename First>
    void setVariadic_(const size_t index, First &&first)
    {
        new (&_data[index]) T(std::forward<First>(first));
        // end of recursiveness
    }

    template <typename OtherT, float OtherIncrement, typename OtherAlloc, bool OtherDebugChecks>
    static constexpr bool equals_(const quickVector *a, const quickVector<OtherT, OtherIncrement, OtherAlloc, OtherDebugChecks> *b)
    {
        return reinterpret_cast<const void *>(a) == reinterpret_cast<const void *>(b);
    }

    static inline void destructItem_(const T &item)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            item.~T();
    }

    void realloc_(const size_t size)
    {
        auto *newData = Alloc::allocate(size);
        for (size_t i = 0; i < _size; i++)
            new (&newData[i]) T(std::move(_data[i]));
        Alloc::deallocate(_data, _capacity);
        _data = newData;
        _capacity = size;
    }

    void assertRange_(const size_t index) const
    {
        if constexpr (DebugChecks)
            fatalAssert(index < _size, "quickVector index out of range");
    }

    void assertDuringForEach() const
    {
        if constexpr (DebugChecks)
            fatalAssert(!_duringForEach.value, "cannot modify quickVector during foreach");
    }

    size_t getIncrementedCapacity_()
    {
        return _capacity == 0 ? 1 : _capacity * Increment;
    }
};
} // namespace engine