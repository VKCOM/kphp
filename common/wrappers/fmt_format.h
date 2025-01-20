// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef COMMON_WRAPPERS_FMT_FORMAT_H
#define COMMON_WRAPPERS_FMT_FORMAT_H

#include "common/wrappers/string_view.h"

#  define fmt_format(format_s, args...) std::format(format_s, ##args)
#  define fmt_print(format_s, args...) std::print(format_s, ##args)
#  define fmt_fprintf(file, format_s, args...) std::print(file, format_s, ##args)
#  define fmt_format_to(out_iter, format_s, args...) std::format_to(out_iter, format_s, ##args)

#endif // COMMON_WRAPPERS_FMT_FORMAT_H
