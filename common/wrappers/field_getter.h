// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_FIELD_GETTER_H
#define ENGINE_FIELD_GETTER_H

namespace vk {

template<class Struct, class Field>
auto make_field_getter(Field Struct::*field) {
  return std::mem_fn(field);
}

} // namespace vk

#endif // ENGINE_FIELD_GETTER_H
