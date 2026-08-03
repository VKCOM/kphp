//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <type_traits>

namespace vk {

template<typename... Types>
inline constexpr bool is_unique_v = true;

template<typename T, typename... Types>
inline constexpr bool is_unique_v<T, Types...> = (!std::is_same_v<T, Types> && ...) && is_unique_v<Types...>;

} // namespace vk
