// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace vk {

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define VK_GCC_SUPPORTS_REF
#endif

template<typename T>
struct remove_cv_ref_for_method {
  using type = T;
};
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...)> {
  using type = R (C::*)(A...);
};
#ifdef VK_GCC_SUPPORTS_REF
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) &&> {
  using type = R (C::*)(A...);
};
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) &> {
  using type = R (C::*)(A...);
};
#endif
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) volatile> {
  using type = R (C::*)(A...);
};
#ifdef VK_GCC_SUPPORTS_REF
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) volatile &&> {
  using type = R (C::*)(A...);
};
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) volatile &> {
  using type = R (C::*)(A...);
};
#endif
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) const> {
  using type = R (C::*)(A...);
};
#ifdef VK_GCC_SUPPORTS_REF
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) const &&> {
  using type = R (C::*)(A...);
};
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) const &> {
  using type = R (C::*)(A...);
};
#endif
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) const volatile> {
  using type = R (C::*)(A...);
};
#ifdef VK_GCC_SUPPORTS_REF
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) const volatile &&> {
  using type = R (C::*)(A...);
};
template<class R, class C, class... A>
struct remove_cv_ref_for_method<R (C::*)(A...) const volatile &> {
  using type = R (C::*)(A...);
};
#endif

#undef VK_GCC_SUPPORTS_REF
} // namespace vk
