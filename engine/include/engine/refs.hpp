#pragma once

namespace
{
    template<typename T>
    struct ptrProxy
    {
        T* ptr;
        size_t refCount;

        ptrProxy(T* value) : ptr(value), refCount(1) {}

        void release()
        {
            delete ptr;
            ptr = nullptr;
        }

        void deleteIfNoRefsLeft()
        {
            if (refCount <= 0)
                delete this;
        }

        ptrProxy<T>* makeCopy() const
        {
            return new ptrProxy<T>(ptr != nullptr ? new T(*ptr) : nullptr);
        }
    };
}

namespace engine
{
    template<typename T>
    struct ref;

    // owning reference
    template<typename T>
    struct refOwn
    {
        friend ref<T>;

        refOwn() : _ptrProxy(nullptr) {}

        refOwn(T* value) : _ptrProxy(new ptrProxy<T>(value)) {}

        // copy
        refOwn(const refOwn<T>& other) : _ptrProxy(other._ptrProxy != nullptr ? other._ptrProxy->makeCopy() : nullptr)
        {
            if (_ptrProxy != nullptr)
                _ptrProxy->refCount++;
        }

        // move
        refOwn(refOwn<T>&& other) noexcept : _ptrProxy(std::exchange(other._ptrProxy, nullptr)) {}

        ~refOwn()
        {
            onDelete_();
        }

        // copy
        refOwn<T>& operator=(const refOwn<T>& other)
        {
            if (this != &other)
            {
                onDelete_();
                new (this) refOwn<T>(other);
            }
            return *this;
        }

        // move
        refOwn<T>& operator=(refOwn<T>&& other) noexcept
        {
            if (this != &other)
            {
                onDelete_();
                new (this) refOwn<T>(std::move(other));
            }
            return *this;
        }


        T* get() const { return _ptrProxy == nullptr ? nullptr : _ptrProxy->ptr; }
        operator T* () const { return get(); }

    private:
        ptrProxy<T>* _ptrProxy = nullptr;

        void onDelete_()
        {
            if (_ptrProxy != nullptr)
            {
                _ptrProxy->release();
                _ptrProxy->refCount--;
                _ptrProxy->deleteIfNoRefsLeft();
            }
        }
    };

    // not owning reference
    template<typename T>
    struct ref
    {
        ref() : _ptrProxy(nullptr) {}

        ref(const refOwn<T>& other) : _ptrProxy(other._ptrProxy)
        {
            if (_ptrProxy != nullptr)
                _ptrProxy->refCount++;
        }

        // copy
        ref(const ref<T>& other) : _ptrProxy(other._ptrProxy)
        {
            if (_ptrProxy != nullptr)
                _ptrProxy->refCount++;
        }

        // move
        ref(ref<T>&& other) noexcept : _ptrProxy(std::exchange(other._ptrProxy, nullptr)) {}

        ~ref()
        {
            onDelete_();
        }

        // copy
        ref<T>& operator=(const ref<T>& other)
        {
            if (this != &other)
            {
                onDelete_();
                new (this) ref<T>(other);
            }
            return *this;
        }

        // move
        ref<T>& operator=(ref<T>&& other) noexcept
        {
            if (this != &other)
            {
                delete this;
                new (this) ref<T>(std::move(other));
            }
            return *this;
        }

        T* get() const { return _ptrProxy == nullptr ? nullptr : _ptrProxy->ptr; }
        operator T* () const { return get(); }

    private:
        ptrProxy<T>* _ptrProxy = nullptr;

        void onDelete_()
        {
            if (_ptrProxy != nullptr)
            {
                _ptrProxy->refCount--;
                _ptrProxy->deleteIfNoRefsLeft();
            }
        }
    };
}