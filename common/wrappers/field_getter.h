// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_FIELD_GETTER_H
#define ENGINE_FIELD_GETTER_H

namespace vk {

template<class Struct, class Field>
struct field_getter {
  const Field Struct::*field;
  const Field &operator()(const Struct &s) {
    return s.*field;
  }
};

template<class Struct, class Field>
field_getter<Struct, Field> make_field_getter(Field Struct::*field) {
  return field_getter<Struct, Field>{field};
}

} // namespace vk

#endif // ENGINE_FIELD_GETTER_H
