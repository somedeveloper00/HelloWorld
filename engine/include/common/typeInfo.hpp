#pragma once

#include <span>
#include <type_traits>

// needed for macros
#include "constexprUtils.hpp"
#include "engine/quickVector.hpp"
#include "engine/ref.hpp"
#include <array>

// disallows multiple components of this type in one entity
// returns false if there was already one component of this type in the entity
#define disallowMultipleComponents(type)                                                                   \
    static engine::quickVector<engine::weakRef<type>> s_buffer##type;                                      \
    getEntity()->getComponents(s_buffer##type);                                                            \
    if (s_buffer##type.size() > 1)                                                                         \
    {                                                                                                      \
        log::logWarning("Entity \"{}\" has more than one \"" #type "\" components, which is not allowed.", \
                        getEntity()->name, getTypeName());                                                 \
        return false;                                                                                      \
    }                                                                                                      \
    s_buffer##type.clear();

// use in components to create type information. This will be used for fast reflection-like purposes
#define createTypeInformation(type, base)                                                                      \
    /* the type of this class's base */                                                                        \
    using baseType = base;                                                                                     \
    /* whether or not this type has a base type */                                                             \
    static constexpr bool c_hasBase = true;                                                                    \
    /* number of bases in the inheritance chain, including this one */                                         \
    static constexpr size_t c_baseCount = 1 + base::c_baseCount;                                               \
    /* hash for this type */                                                                                   \
    static constexpr size_t c_typeHash = getTypeHash_<type>();                                                 \
    /* list of hashes for this type and all its bases. self starts at 0 and goes up in index with hierarchy */ \
    static constexpr std::array<size_t, c_baseCount> c_typeHashes = insert(base::c_typeHashes, c_typeHash, 0); \
    std::span<const size_t> getTypeHashes() const noexcept override                                            \
    {                                                                                                          \
        return {c_typeHashes.data(), c_baseCount};                                                             \
    }                                                                                                          \
    const std::string &getTypeName() const noexcept override                                                   \
    {                                                                                                          \
        const static std::string s_typeName = typeid(type).name();                                             \
        return s_typeName;                                                                                     \
    }                                                                                                          \
    const size_t &getTypeHash() const noexcept override                                                        \
    {                                                                                                          \
        return c_typeHash;                                                                                     \
    }

#define createBaseTypeInformation(type)                                                                        \
    /* whether or not this type has a base type */                                                             \
    static constexpr bool c_hasBase = false;                                                                   \
    /* number of bases in the inheritance chain, including this one */                                         \
    static constexpr size_t c_baseCount = 1;                                                                   \
    /* hash for this type */                                                                                   \
    static constexpr size_t c_typeHash = getTypeHash_<type>();                                                 \
    /* list of hashes for this type and all its bases. self starts at 0 and goes up in index with hierarchy */ \
    static constexpr std::array<size_t, c_baseCount> c_typeHashes = {c_typeHash};                              \
    /* get type hashes for this type */                                                                        \
    virtual std::span<const size_t> getTypeHashes() const noexcept                                             \
    {                                                                                                          \
        return {c_typeHashes.data(), c_baseCount};                                                             \
    }                                                                                                          \
    /* get the name of the type (only usable in debug mode) */                                                 \
    virtual const std::string &getTypeName() const noexcept                                                    \
    {                                                                                                          \
        const static std::string s_typeName = typeid(type).name();                                             \
        return s_typeName;                                                                                     \
    }                                                                                                          \
    virtual const size_t &getTypeHash() const noexcept                                                         \
    {                                                                                                          \
        return c_typeHash;                                                                                     \
    }

// returns whether or not the object is of type TargetType
template <typename TargetType, typename ObjectType>
constexpr bool isOfType(const ObjectType &object)
{
    if constexpr (std::is_same_v<TargetType, ObjectType>)
        return true;
    static_assert(std::is_base_of_v<ObjectType, TargetType>, "no need to check for type when target type is not derived from object type");
    const std::span<const size_t> hashes = object.getTypeHashes();
    for (size_t i = 0; i < hashes.size(); i++)
        if (hashes[i] == TargetType::c_typeHash)
            return true;
    return false;
}