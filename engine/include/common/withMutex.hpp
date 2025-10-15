#pragma once

#include <mutex>

// a variable with a mutex packed into one variable
template <typename T>
struct withMutex final
{
    // shortcut for mutex::lock() and executing logic and then mutex::unlock()
    template <typename Func>
    inline void execute(Func &&func)
    {
        _mutex.lock();
        func(_value);
        _mutex.unlock();
    }

    // shortcut for mutex::lock() and setting value's item at an index and then mutex::unlock()
    template <typename Value>
    inline void setOnIndex(const size_t index, Value &&value)
    {
        _mutex.lock();
        _value[index] = std::forward<Value>(value);
        _mutex.unlock();
    }

    // shortcut for mutex::lock() and returns the value
    inline T &lock()
    {
        _mutex.lock();
        return _value;
    }

    // shortcut for mutex::unlock()
    inline void unlock()
    {
        _mutex.unlock();
    }

  private:
    T _value;
    std::mutex _mutex;
};