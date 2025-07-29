#pragma once

namespace
{
    template<typename T>
    struct ptrPlace
    {
        T* ptr;
        size_t refCount;

        ptrPlace(T* value) : ptr(value), refCount(1) {}

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

        ptrPlace<T>* makeCopy() const
        {
            return new ptrPlace<T>(ptr != nullptr ? new T(*ptr) : nullptr);
        }
    };
}

namespace engine
{
    template<typename T, bool Owner = false>
    struct ref {};

    // owning reference
    template<typename T>
    struct ref<T, true>
    {
        friend ref<T, false>;

        ref() : _ptrPlace(nullptr) {}

        ref(T* value) : _ptrPlace(new ptrPlace<T>(value)) {}

        // copy
        ref(const ref<T, true>& other) : _ptrPlace(other._ptrPlace != nullptr ? other._ptrPlace->makeCopy() : nullptr)
        {
            if (_ptrPlace != nullptr)
                _ptrPlace->refCount++;
        }

        // move
        ref(ref<T, true>&& other) noexcept : _ptrPlace(std::exchange(other._ptrPlace, nullptr)) {}

        ~ref() { onDelete_(); }

        // copy
        ref<T, true>& operator=(const ref<T, true>& other)
        {
            if (this != &other)
            {
                onDelete_();
                new (this) ref<T, true>(other);
            }
            return *this;
        }

        // move
        ref<T, true>& operator=(ref<T, true>&& other) noexcept
        {
            if (this != &other)
            {
                onDelete_();
                new (this) ref<T, true>(std::move(other));
            }
            return *this;
        }


        T* get() const { return _ptrPlace == nullptr ? nullptr : _ptrPlace->ptr; }
        operator T* () const { return get(); }

    private:
        ptrPlace<T>* _ptrPlace = nullptr;

        void onDelete_()
        {
            if (_ptrPlace != nullptr)
            {
                _ptrPlace->release();
                _ptrPlace->refCount--;
                _ptrPlace->deleteIfNoRefsLeft();
            }
        }
    };

    // not owning reference
    template<typename T>
    struct ref<T, false>
    {
        ref() : _ptrPlace(nullptr) {}

        template<bool Owner2>
        ref(const ref<T, Owner2>& other) : _ptrPlace(other._ptrPlace)
        {
            if (_ptrPlace != nullptr)
                _ptrPlace->refCount++;
        }

        ref(ref<T, true>&& other) noexcept : _ptrPlace(std::exchange(other._ptrPlace, nullptr)) {}

        ~ref()
        {
            onDelete_();
        }

        // copy
        ref<T, false>& operator=(const ref<T, true>& other)
        {
            onDelete_();
            new (this) ref<T, false>(other);
            return *this;
        }

        // copy
        ref<T, false>& operator=(const ref<T, false>& other)
        {
            if (this != &other)
            {
                onDelete_();
                new (this) ref<T, false>(other);
            }
            return *this;
        }

        // move
        template<bool Owner2>
        ref<T, false>& operator=(ref<T, Owner2>&& other) noexcept
        {
            if (this != &other)
            {
                delete this;
                new (this) ref<T, false>(std::move(other));
            }
            return *this;
        }

        T* get() const { return _ptrPlace == nullptr ? nullptr : _ptrPlace->ptr; }
        operator T* () const { return get(); }

    private:
        ptrPlace<T>* _ptrPlace = nullptr;

        void onDelete_()
        {
            if (_ptrPlace != nullptr)
            {
                _ptrPlace->refCount--;
                _ptrPlace->deleteIfNoRefsLeft();
            }
        }
    };

    // owning reference
    template<typename T>
    using refOwn = ref<T, true>;
}