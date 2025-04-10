// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef COMMON_WRAPPERS_FMT_FORMAT_H
#define COMMON_WRAPPERS_FMT_FORMAT_H

#include <fmt/core.h>

#if __cplusplus <= 201703 || FMT_VERSION < 70000
#undef FAST_COMPILATION_FMT
#endif

#ifndef FAST_COMPILATION_FMT
#include <fmt/format.h>
#endif

#include "common/wrappers/string_view.h"

#ifdef FMT_STRING
#define fmt_format(format_s, args...) fmt::format(FMT_STRING(format_s), ##args)
#define fmt_print(format_s, args...) fmt::print(FMT_STRING(format_s), ##args)
#if FMT_VERSION < 60000
#define fmt_fprintf(file, format_s, args...) fmt::print(file, "{}", fmt_format(format_s, ##args))
#define fmt_format_to(iter, format_s, args...) fmt::format_to(iter, "{}", fmt_format(format_s, ##args))
#else
#define fmt_fprintf(file, format_s, args...) fmt::print(file, FMT_STRING(format_s), ##args)
#define fmt_format_to(iter, format_s, args...) fmt::format_to(iter, FMT_STRING(format_s), ##args)
#endif
#else
#define fmt_format(format_s, args...) fmt::format(format_s, ##args)
#define fmt_print(format_s, args...) fmt::print(format_s, ##args)
#define fmt_fprintf(file, format_s, args...) fmt::print(file, format_s, ##args)
#define fmt_format_to(out_iter, format_s, args...) fmt::format_to(out_iter, format_s, ##args)
#endif

#ifndef FAST_COMPILATION_FMT
namespace fmt {
template<>
struct formatter<vk::string_view> : fmt::formatter<fmt::string_view> {
  template<typename FormatCtx>
  auto format(vk::string_view a, FormatCtx& ctx) {
    return fmt::formatter<fmt::string_view>::format(fmt::string_view{a.data(), a.size()}, ctx);
  }
};
} // namespace fmt
#endif

#endif // COMMON_WRAPPERS_FMT_FORMAT_H
