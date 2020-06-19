#ifndef LAMBDIFIER_DETAIL_TYPE_TRAITS_HPP
#define LAMBDIFIER_DETAIL_TYPE_TRAITS_HPP

#include <type_traits>

namespace lambdifier::detail
{

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class T, class U>
concept same_helper = std::is_same_v<T, U>;

template <class T, class U>
concept same_as = same_helper<T, U> &&same_helper<U, T>;

} // namespace lambdifier::detail

#endif
