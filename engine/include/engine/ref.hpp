#pragma once

#include "alloc.hpp"
#include <cstdint>
#include <process.h>
#include <tracy/Tracy.hpp>
#include <type_traits>
#include <utility>

namespace
{
// holds object pointer and reference counters
struct holder
{
    void *objectPtr;

    // counter for owner refs
    uint16_t ownerCount;

    // counter for all refs (owner or not)
    uint16_t totalCount;
};
} // namespace

namespace engine
{
// holds a reference to an object and uses tracy profiler
template <typename T, bool Owner, typename Alloc = rawAlloc<T>>
struct ref
{
    template <typename, bool, typename>
    friend struct ref;

    // initialize empty weak ref
    ref()
        : _holderPtr(nullptr)
    {
        static_assert(!Owner, "cannot empty-initialize an owning ref");
    }

    // create a new object and keep reference
    // the extra bool parameter is just to differentiate from the empty constructor
    template <typename... Args>
        requires Owner
    ref(bool, Args &&...args)
        : _holderPtr(new holder{new T(std::forward<Args>(args)...), 1, 1})
    {
        TracyAlloc(_holderPtr->objectPtr, sizeof(T));
    }

    // move ref object
    ref(ref<T, Owner> &&other) noexcept
        requires(!Owner) // cannot move an owning ref
        : _holderPtr(std::exchange(other._holderPtr, nullptr))
    {
    }

    // copy reference (basic form | otherwise it won't compile)
    ref(const ref &other)
        : _holderPtr(other.getOtherPtrSafe_())
    {
        onAddHolderSafe_();
    }

    // copy reference
    template <typename OtherT, bool OtherOwner>
        requires(canCopyFrom_<OtherT, OtherOwner>())
    ref(const ref<OtherT, OtherOwner> &other)
        : _holderPtr(other.getOtherPtrSafe_())
    {
        onAddHolderSafe_();
    }

    ~ref()
    {
        onRemoveHolderSafe_();
    }

    // copy reference (basic form | otherwise it won't compile)
    ref &operator=(const ref &other)
    {
        if (!equals_(this, &other))
        {
            onRemoveHolderSafe_();
            _holderPtr = other._holderPtr;
            onAddHolderSafe_();
        }
        return *this;
    }

    // copy reference
    template <typename OtherT, bool OtherOwner>
        requires(canCopyFrom_<OtherT, OtherOwner>())
    ref<T, Owner> &operator=(const ref<OtherT, OtherOwner> &other)
    {
        if (!equals_(this, &other))
        {
            onRemoveHolderSafe_();
            _holderPtr = other._holderPtr;
            onAddHolderSafe_();
        }
        return *this;
    }

    // move ref object
    ref &operator=(ref &&other) noexcept
    {
        if (!equals_(this, &other))
        {
            onRemoveHolderSafe_();
            _holderPtr = std::exchange(other._holderPtr, nullptr);
        }
        return *this;
    }

    operator bool() const noexcept
    {
        static_assert(!Owner, "owner refs don't need validity checks. they're always valid");
        return this != nullptr && _holderPtr && _holderPtr->objectPtr;
    }

    operator T &() const noexcept
        requires(Owner) // to avoid dereferencing an null pointer
    {
        return *static_cast<T *>(_holderPtr->objectPtr);
    }

    operator T *() const noexcept
    {
        return getObjectPointerSafe_();
    }

    T *operator->() const noexcept
    {
        return getObjectPointerSafe_();
    }

    template <typename OtherT, bool OtherOwner>
    bool operator==(const ref<OtherT, OtherOwner> &other) const noexcept
    {
        return equals_(this, &other);
    }

    template <typename OtherT, bool OtherOwner>
    bool operator!=(const ref<OtherT, OtherOwner> &other) const noexcept
    {
        return !equals_(this, &other);
    }

    // casts to another ref type. returns empty ref if this instance is an empty ref already.
    // uses reinterpret_cast, so no runtime checks are performed
    template <typename OtherT>
    ref<OtherT, Owner> *as() const
    {
        if constexpr (!Owner)
            if (!this)
                return nullptr;
        return new ref<OtherT, Owner>{_holderPtr};
    }

  private:
    holder *_holderPtr;

    ref(holder *holder)
        : _holderPtr(holder)
    {
    }

    void onRemoveHolderSafe_()
    {
        if constexpr (Owner)
            onRemoveHolder_();
        else
        {
            if (this != nullptr && _holderPtr)
                onRemoveHolder_();
        }
    }

    void onRemoveHolder_()
    {
        TracyFree(_holderPtr);
        if constexpr (Owner)
            if (--_holderPtr->ownerCount == 0)
                // delete the actual object (non owning refs may still reference the nullptr pointer)
                delete static_cast<T *>(_holderPtr->objectPtr);
        if (--_holderPtr->totalCount == 0)
            // delete the holder itself
            delete _holderPtr;
    }

    void onAddHolderSafe_()
    {
        if constexpr (Owner)
            onAddHolder_();
        else
        {
            if (this != nullptr && _holderPtr)
                onAddHolder_();
        }
    }

    void onAddHolder_()
    {
        if constexpr (Owner)
            _holderPtr->ownerCount++;
        _holderPtr->totalCount++;
    }

    holder *getOtherPtrSafe_() const
    {
        if constexpr (Owner)
            return _holderPtr;
        return this != nullptr ? _holderPtr : nullptr;
    }

    T *getObjectPointerSafe_() const
    {
        if constexpr (Owner)
            return static_cast<T *>(_holderPtr->objectPtr);
        return this && _holderPtr ? static_cast<T *>(_holderPtr->objectPtr) : nullptr;
    }

    template <typename OtherT, bool OtherOwner>
    static constexpr bool canCopyFrom_()
    {
        return (OtherOwner || !Owner) &&     // cannot copy non-owning to owning (owning refs should always be valid)
               std::is_base_of_v<T, OtherT>; // safe upcasting for inheritence
    }

    template <typename OtherT, bool OtherOwner>
    static constexpr bool equals_(const ref *a, const ref<OtherT, OtherOwner> *b)
    {
        return a->getOtherPtrSafe_() == b->getOtherPtrSafe_();
    }
};

template <typename T>
using weakRef = ref<T, false>;

template <typename T>
using ownRef = ref<T, true>;
} // namespace engine