#ifndef SF_SUPPORT_SET_HPP
#define SF_SUPPORT_SET_HPP

#include <type_traits> // true_type, false_type

#include <set> // set, multiset
#include <unordered_set> // unordered_set, unordered_multiset

#include <utility> // move

#include <SF/Core/TypeRegistry.hpp>
#include <SF/Core/TypeCore.hpp>

#include <SF/Compress.hpp>
#include <SF/ExternSerialization.hpp>

#define _SF_IS_STD_SET_TYPE_META_GENERIC(set_type)                                                   \
    template <typename> struct is_std_##set_type : std::false_type {};                                  \
    template <typename Key, typename Compare, typename Alloc>                                           \
    struct is_std_##set_type<std::set_type<Key, Compare, Alloc>> : std::true_type {};

namespace sf
{

namespace meta
{

_SF_IS_STD_SET_TYPE_META_GENERIC(set)
_SF_IS_STD_SET_TYPE_META_GENERIC(unordered_set)
_SF_IS_STD_SET_TYPE_META_GENERIC(multiset)
_SF_IS_STD_SET_TYPE_META_GENERIC(unordered_multiset)

template <class T> constexpr bool is_std_any_unordered_set() noexcept
{
    return is_std_unordered_set<T>::value
        or is_std_unordered_multiset<T>::value;
}

template <class T> constexpr bool is_std_any_set() noexcept
{
    return is_std_set<T>::value
        or is_std_multiset<T>::value
        or is_std_any_unordered_set<T>();
}

} // namespace meta

namespace detail
{

template <class T,
          SIREQUIRE(not meta::is_std_any_unordered_set<T>())>
void reserve_unordered(T& ordered, std::size_t size) noexcept { /*pass*/ }

template <class T,
          SIREQUIRE(meta::is_std_any_unordered_set<T>())>
void reserve_unordered(T& unordered, std::size_t size)
{
    unordered.reserve(size);
}

} // namespace detail

inline namespace library
{

EXTERN_CONDITIONAL_SERIALIZATION(Save, set, meta::is_std_any_set<T>())
{
    let::u64 size = set.size();
    archive & size;

    compress::slow(archive, set);

    return archive;
}

EXTERN_CONDITIONAL_SERIALIZATION(Load, set, meta::is_std_any_set<T>())
{
    using value_type = typename T::value_type;

    let::u64 size{};
    archive & size;

    set.clear();
    detail::reserve_unordered(set, size);

    auto hint = set.begin();
    for (let::u64 i = 0; i < size; ++i)
    {
        value_type item{}; // temp
        archive & item;

        hint = set.emplace_hint(hint, std::move(item));
    }

    return archive;
}

} // inline namespace library

} // namespace sf

CONDITIONAL_TYPE_REGISTRY(meta::is_std_any_set<T>())

// clean up
#undef _SF_IS_STD_SET_TYPE_META_GENERIC

#endif // SF_SUPPORT_SET_HPP
