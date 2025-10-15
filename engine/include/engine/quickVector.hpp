#pragma once

#include "alloc.hpp"
#include "engine/errorHandling.hpp"
#include "engine/template.hpp"
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
// A simple vector implementation like std::vector but without iterators and without default initialization of data
template <typename T, float Increment = 2.f, typename Alloc = rawAlloc<T>, bool DebugChecks = isDebugMode()>
struct quickVector
{
    static_assert(Increment > 1, "Increment must be greater than 1");

    template <typename, float, typename, bool>
    friend struct quickVector;

    // gets the templated Increment of this vector
    static constexpr float IncrementValue = Increment;

    quickVector() = default;

    // initialize vector with initial capacity (length of internal array) and optionally the initial size
    quickVector(const size_t initialCapacity, const size_t size = 0)
        : _size(size), _capacity(initialCapacity)
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
        modificationContextBegin_();
        setVariadic_(0, std::forward<TObjects>(initObjects)...);
        modificationContextEnd_();
    }

    // copy reference (basic form | otherwise it won't compile)
    quickVector(const quickVector &other)
        : _size(other._size), _capacity(other._capacity)
    {
        _data = Alloc::allocate(_capacity);
        modificationContextBegin_();
        for (size_t i = 0; i < _size; i++)
            new (&_data[i]) T(other._data[i]);
        modificationContextEnd_();
    }

    // copy reference
    template <typename OtherT, float OtherIncrement>
        requires std::is_convertible_v<OtherT, T>
    quickVector(const quickVector<OtherT, OtherIncrement> &other)
        : _size(other._size), _capacity(other._capacity)
    {
        _data = Alloc::allocate(_capacity);
        modificationContextBegin_();
        for (size_t i = 0; i < _size; i++)
            new (&_data[i]) T(other._data[i]);
        modificationContextEnd_();
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
        modificationContextBegin_();
        if (!equals_(this, &other))
        {
            realloc_(other._capacity);
            _size = other._size;
            for (size_t i = 0; i < _size; i++)
                new (&_data[i]) T(other._data[i]);
        }
        modificationContextEnd_();
        return *this;
    }

    // copy reference
    template <float OtherIncrement>
    quickVector &operator=(const quickVector<T, OtherIncrement> &other)
    {
        modificationContextBegin_();
        if (!equals_(this, &other))
        {
            realloc_(other._capacity);
            _size = other._size;
            for (size_t i = 0; i < _size; i++)
                new (&_data[i]) T(other._data[i]);
        }
        modificationContextEnd_();
        return *this;
    }

    // move reference (basic form | otherwise it won't compile)
    quickVector &operator=(quickVector &&other) noexcept
    {
        modificationContextBegin_();
        if (!equals_(this, &other))
        {
            Alloc::deallocate(_data, _capacity);
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0);
            _capacity = std::exchange(other._capacity, 0);
        }
        modificationContextEnd_();
        return *this;
    }

    // move reference
    template <float OtherIncrement>
    quickVector &operator=(quickVector<T, OtherIncrement> &&other) noexcept
    {
        modificationContextBegin_();
        if (!equals_(this, &other))
        {
            Alloc::deallocate(_data, _capacity);
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0);
            _capacity = std::exchange(other._capacity, 0);
        }
        modificationContextEnd_();
        return *this;
    }

    ~quickVector()
    {
        if (_data)
        {
            modificationContextBegin_();
            for (size_t i = 0; i < _size; ++i)
                destructItem_(_data[i]);
            modificationContextEnd_();
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
        modificationContextBegin_();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        new (&_data[_size++]) T(value);
        modificationContextEnd_();
    }

    // it's faster than multiple push_back
    void push_backRange(const T *ptr, const size_t count)
    {
        modificationContextBegin_();
        while (_size + count > _capacity)
            realloc_(getIncrementedCapacity_());
        for (size_t i = 0; i < count; i++)
            new (&_data[_size + i]) T(std::move(ptr[i]));
        _size += count;
        modificationContextEnd_();
    }

    void push_back(const T &&value)
    {
        modificationContextBegin_();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        new (&_data[_size++]) T(std::move(value));
        modificationContextEnd_();
    }

    template <typename... Args>
    void emplace_back(Args &&...args)
    {
        modificationContextBegin_();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        new (&_data[_size++]) T(std::forward<Args>(args)...);
        modificationContextEnd_();
    }

    // ArgGetters: has one argument size_t which is the index. should return an argument for the final T
    template <typename... ArgGetters>
    void emplace_backRange(const size_t count, ArgGetters &&...argGetters)
    {
        modificationContextBegin_();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        for (size_t i = 0; i < count; i++)
            new (&_data[_size++]) T(ArgGetters(i)...);
        modificationContextEnd_();
    }

    void pop_back()
    {
        modificationContextBegin_();
        assertRange_(0);
        destructItem_(_data[_size - 1]);
        _size--;
        modificationContextEnd_();
    }

    T pop_back_get()
    {
        modificationContextBegin_();
        assertRange_(0);
        T data = std::move(_data[_size - 1]);
        destructItem_(_data[_size - 1]); // might not be necessary but just in case
        _size--;
        modificationContextEnd_();
        return data;
    }

    const size_t &getCapacity() const noexcept
    {
        return _capacity;
    }

    void reserve(const size_t capacity)
    {
        modificationContextBegin_();
        if (capacity > _capacity)
            realloc_(capacity);
        modificationContextEnd_();
    }

    void setSize(const size_t size)
    {
        modificationContextBegin_();
        if constexpr (DebugChecks)
            fatalAssert(size <= _capacity, "cannot set the size to a value greater than the vector's capacity");
        _size = size;
        modificationContextEnd_();
    }

    template <typename Value>
    void insert(const size_t index, Value &&item)
    {
        modificationContextBegin_();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        _size++;
        assertRange_(index);
        for (size_t i = _size; i-- > index + 1;)
            new (&_data[i]) T(std::move(_data[i - 1]));
        new (&_data[index]) T(std::move(item));
        modificationContextEnd_();
    }

    // it's faster to use this than multiple insert
    void insertRange(const size_t index, const T *ptr, const size_t count)
    {
        modificationContextBegin_();
        while (_size + count > _capacity)
            realloc_(getIncrementedCapacity_());
        _size += count;
        assertRange_(index + count - 1);
        for (size_t i = _size; i-- > index + 1;)
            new (&_data[i]) T(std::move(_data[i - 1]));
        for (size_t i = index; i < index + count; i++)
            new (&_data[index]) T(std::move(ptr[i]));
        modificationContextEnd_();
    }

    template <typename... Args>
    void emplace(const size_t index, Args &&...args)
    {
        modificationContextBegin_();
        if (_size + 1 > _capacity)
            realloc_(getIncrementedCapacity_());
        _size++;
        assertRange_(index);
        for (size_t i = _size; i-- > index + 1;)
            new (&_data[i]) T(std::move(_data[i - 1]));
        new (&_data[index]) T(std::forward<Args>(args)...);
        modificationContextEnd_();
    }

    void clear()
    {
        modificationContextBegin_();
        for (size_t i = 0; i < _size; ++i)
            destructItem_(_data[i]);
        _size = 0;
        modificationContextEnd_();
    }

    template <typename Func>
    void forEachAndClear(Func &&func)
    {
        modificationContextBegin_();
        for (size_t i = 0; i < _size; ++i)
            func(_data[i]);
        _size = 0;
        modificationContextEnd_();
    }

    // will be invalid if there's no items
    T &back()
    {
        modificationContextBegin_();
        modificationContextEnd_();
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
        loopContextBegin_();
        for (size_t i = 0; i < _size; ++i)
            func(_data[i]);
        loopContextEnd_();
    }

    template <typename Func>
    void forEach(Func &&func) const
    {
        for (size_t i = 0; i < _size; ++i)
            func(static_cast<const T &>(_data[i]));
    }

    template <typename Func>
    void forEachParallel(Func &&func)
    {
        loopContextBegin_();
#pragma omp parallel for
        for (signed long long i = 0; i < _size; ++i)
            func(_data[i]);
        loopContextEnd_();
    }

    template <typename Func>
    void forEachParallel(Func &&func) const
    {
#pragma omp parallel for
        for (signed long long i = 0; i < _size; ++i)
            func(static_cast<const T &>(_data[i]));
    }

    // 0: index as size_t, 1: item
    template <typename Func>
    void forEachIndexed(Func &&func)
    {
        loopContextBegin_();
        for (size_t i = 0; i < _size; ++i)
            func(i, _data[i]);
        loopContextEnd_();
    }

    template <typename Func>
    void forEachIndexed(Func &&func) const
    {
        for (size_t i = 0; i < _size; ++i)
            func(i, static_cast<const T &>(_data[i]));
    }

    template <typename Func>
    void forEachIndexedParallel(Func &&func)
    {
        loopContextBegin_([&]() {
#pragma omp parallel for
            for (signed long long i = 0; i < _size; ++i)
                func(i, _data[i]);
        });
    }

    template <typename Func>
    void forEachIndexedParallel(Func &&func) const
    {
#pragma omp parallel for
        for (signed long long i = 0; i < _size; ++i)
            func(i, static_cast<const T &>(_data[i]));
    }

    void erase(const T &item)
    {
        modificationContextBegin_();
        size_t write = 0;
        for (size_t read = 0; read < _size; ++read)
            if (!(_data[read] == item))
            {
                if (write != read)
                    _data[write] = std::move(_data[read]);
                ++write;
            }
            else
                destructItem_(_data[read]);
        _size = write;
        modificationContextEnd_();
    }

    template <typename Func>
    void eraseIf(Func &&func)
    {
        modificationContextBegin_();
        size_t write = 0;
        for (size_t read = 0; read < _size; ++read)
            if (!func(_data[read]))
            {
                if (write != read)
                    _data[write] = std::move(_data[read]);
                ++write;
            }
            else if constexpr (!std::is_pointer_v<T>)
                destructItem_(_data[read]);
        _size = write;
        modificationContextEnd_();
    }

    // faster than eraseIf, but order or items is not guaranteed to be kept in the end
    template <typename Func>
    void eraseIfUnordered(Func &&func)
    {
        modificationContextBegin_();
        for (size_t i = 0; i < _size;)
            if (func(_data[i]))
            {
                destructItem_(_data[i]);
                _data[i] = std::move(_data[--_size]);
            }
            else
                ++i;
        modificationContextEnd_();
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
        modificationContextBegin_();
        modificationContextEnd_();
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
    ConditionalVariable<bool, false, DebugChecks> _duringModification;
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

    size_t getIncrementedCapacity_()
    {
        return _capacity == 0 ? 1 : _capacity * Increment;
    }

    // during DEBUG, it'll guard against start a new modification while another is ongoing and while a foreach is ongoing
    void modificationContextBegin_()
    {
        if constexpr (DebugChecks)
        {
            fatalAssert(!_duringModification.value, "cannot modify quickVector during modification.");
            fatalAssert(!_duringForEach.value, "cannot modify quickVector during foreach");
        }
        if constexpr (DebugChecks)
            _duringModification.value = true;
    };

    // comes after modificationContextBegin_'s context is finsihed
    void modificationContextEnd_()
    {
        if constexpr (DebugChecks)
            _duringModification.value = false;
    }

    // same as modificationContext_ but returns a value
    // during DEBUG, it'll guard against start a new modification while a loop is ongoing
    void loopContextBegin_()
    {
        if constexpr (DebugChecks)
            fatalAssert(!_duringModification.value, "cannot loop quickVector during modification.");
        if constexpr (DebugChecks)
            _duringForEach.value = true;
    };

    // comes after loopContextBegin_'s context is finished
    void loopContextEnd_()
    {
        if constexpr (DebugChecks)
            _duringForEach.value = false;
    }
};

// a version of quickVector that initializes all bytes to zero (uses calloc)
template <typename T>
using quickVectorZeroInitialize = quickVector<T, quickVector<T>::IncrementValue, rawAlloc<T, allocType::callocAllocType>>;
} // namespace engine
