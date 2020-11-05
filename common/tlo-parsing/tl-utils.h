// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>

#include "common/tlo-parsing/tl-objects.h"

namespace vk {
namespace tl {
  inline vk::tl::type *get_type_of(const type_expr *expr, const vk::tl::tl_scheme *scheme) {
    vk::tl::type *res = scheme->get_type_by_magic(expr->type_id);
    assert(res);
    return res;
  }

  inline vk::tl::type *get_type_of(const combinator *constructor, const vk::tl::tl_scheme *scheme) {
    assert(constructor->is_constructor());
    vk::tl::type *res = scheme->get_type_by_magic(constructor->type_id);
    assert(res);
    return res;
  }
}
}
