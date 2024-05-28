// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

namespace tl2cpp {
// Generated code example:
/*
 * Untyped TL:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::store(const mixed &tl_object) {
  const string &c_name = tl_arr_get(tl_object, tl_str$_, 0, -2147483553).to_string();
  if (c_name == tl_str$left) {
    f$store_int(0x0a29cd5d);
    c_left<T0, inner_magic0, T1, inner_magic1>::store(tl_object, std::move(X), std::move(Y));
  } else if (c_name == tl_str$right) {
    f$store_int(0xdf3ecb3b);
    c_right<T0, inner_magic0, T1, inner_magic1>::store(tl_object, std::move(X), std::move(Y));
  } else {
    get_component_context()->rpc_component_context.current_query.raise_storing_error("Invalid constructor %s of type %s", c_name.c_str(), "Either");
  }
}
 * Typed TL:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::typed_store(const PhpType &tl_object) {
  if (f$is_a<typename left__<typename T0::PhpType, typename T1::PhpType>::type>(tl_object)) {
    f$store_int(0x0a29cd5d);
    const typename left__<typename T0::PhpType, typename T1::PhpType>::type *conv_obj = tl_object.template cast_to<typename left__<typename T0::PhpType, typename T1::PhpType>::type>().get();
    c_left<T0, inner_magic0, T1, inner_magic1>::typed_store(conv_obj, std::move(X), std::move(Y));
  } else if (f$is_a<typename right__<typename T0::PhpType, typename T1::PhpType>::type>(tl_object)) {
    f$store_int(0xdf3ecb3b);
    const typename right__<typename T0::PhpType, typename T1::PhpType>::type *conv_obj = tl_object.template cast_to<typename right__<typename T0::PhpType, typename T1::PhpType>::type>().get();
    c_right<T0, inner_magic0, T1, inner_magic1>::typed_store(conv_obj, std::move(X), std::move(Y));
  } else {
    get_component_context()->rpc_component_context.current_query.raise_storing_error("Invalid constructor %s of type %s", tl_object.get_class(), "Either");
  }
}
*/
struct TypeStore {
  const vk::tlo_parsing::type *type;
  std::string template_str;
  bool typed_mode;

  TypeStore(const vk::tlo_parsing::type *type, std::string template_str, bool typed_mode = false) :
    type(type),
    template_str(std::move(template_str)),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const;
};


// Generated code example:
/*
 * Untyped TL:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
array<mixed> t_Either<T0, inner_magic0, T1, inner_magic1>::fetch() {
  array<mixed> result;
  CHECK_EXCEPTION(return result);
  auto magic = static_cast<unsigned int>(rpc_fetch_int());
  switch(magic) {
    case 0x0a29cd5d: {
      result = c_left<T0, inner_magic0, T1, inner_magic1>::fetch(std::move(X), std::move(Y));
      result.set_value(tl_str$_, tl_str$left, -2147483553);
      break;
    }
    case 0xdf3ecb3b: {
      result = c_right<T0, inner_magic0, T1, inner_magic1>::fetch(std::move(X), std::move(Y));
      result.set_value(tl_str$_, tl_str$right, -2147483553);
      break;
    }
    default: {
      get_component_context()->rpc_component_context.current_query.raise_fetching_error("Incorrect magic of type Either: 0x%08x", magic);
    }
  }
  return result;
}
 * Typed TL:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::typed_fetch_to(PhpType &tl_object) {
  CHECK_EXCEPTION(return);
  auto magic = static_cast<unsigned int>(rpc_fetch_int());
  switch(magic) {
    case 0x0a29cd5d: {
      class_instance<typename left__<typename T0::PhpType, typename T1::PhpType>::type> result;
      result.alloc();
      c_left<T0, inner_magic0, T1, inner_magic1>::typed_fetch_to(result.get(), std::move(X), std::move(Y));
      tl_object = result;
      break;
    }
    case 0xdf3ecb3b: {
      class_instance<typename right__<typename T0::PhpType, typename T1::PhpType>::type> result;
      result.alloc();
      c_right<T0, inner_magic0, T1, inner_magic1>::typed_fetch_to(result.get(), std::move(X), std::move(Y));
      tl_object = result;
      break;
    }
    default: {
      get_component_context()->rpc_component_context.current_query.raise_fetching_error("Incorrect magic of type Either: 0x%08x", magic);
    }
  }
}
*/
struct TypeFetch {
  const vk::tlo_parsing::type *type;
  std::string template_str;
  bool typed_mode;

  inline TypeFetch(const vk::tlo_parsing::type *type, std::string template_str, bool typed_mode = false) :
    type(type),
    template_str(std::move(template_str)),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const;
};


/*
 * Typed TL example:
template <typename, typename>
struct Either__ {
  using type = tl_undefined_php_type;
};

template <>
struct Either__<C$VK$TL$Types$Either__string__graph_Vertex::X, C$VK$TL$Types$Either__string__graph_Vertex::Y> {
  using type = C$VK$TL$Types$Either__string__graph_Vertex;
};

template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
struct t_Either {
  using PhpType = class_instance<typename Either__<typename T0::PhpType, typename T1::PhpType>::type>;
  T0 X;
  T1 Y;
  explicit t_Either(T0 X, T1 Y) : X(std::move(X)), Y(std::move(Y)) {}

  void store(const mixed& tl_object);
  array<mixed> fetch();
  void typed_store(const PhpType &tl_object);
  void typed_fetch_to(PhpType &tl_object);
};
*/
struct TlTypeDeclaration {
  const vk::tlo_parsing::type *t;

  // 'messages.ChatInfoUser' TL type can be either:
  // * A VK\TL\Types\messages\chatInfoUser PHP class
  // * A VK\TL\Types\messages\ChatInfoUser PHP interface if the type is polymorphic
  static bool does_tl_type_need_typed_fetch_store(const vk::tlo_parsing::type *t) {
    if (t->name == "ReqResult") {
      // the site won't compile without it unless the typed rpc is included as t_ReqResult is not a compilation target
      bool typed_php_code_exists = !!G->get_class(vk::tl::PhpClasses::rpc_response_ok_with_tl_full_namespace());
      return typed_php_code_exists;
    }
    return !get_all_php_classes_of_tl_type(t).empty();
  }

  explicit TlTypeDeclaration(const vk::tlo_parsing::type *t) :
    t(t) {}

  void compile(CodeGenerator &W) const;
};

struct TlTypeDefinition {
  const vk::tlo_parsing::type *t;

  explicit TlTypeDefinition(const vk::tlo_parsing::type *t) :
    t(t) {}

  void compile(CodeGenerator &W) const;
};
}
