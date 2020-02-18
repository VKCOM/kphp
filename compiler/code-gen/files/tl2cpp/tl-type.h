#pragma once
#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

namespace tl2cpp {
// Пример сгенеренного кода:
/*
 * Нетипизированно:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::store(const var &tl_object) {
  const string &c_name = tl_arr_get(tl_object, tl_str$_, 0, -2147483553).to_string();
  if (c_name == tl_str$left) {
    f$store_int(0x0a29cd5d);
    c_left<T0, inner_magic0, T1, inner_magic1>::store(tl_object, std::move(X), std::move(Y));
  } else if (c_name == tl_str$right) {
    f$store_int(0xdf3ecb3b);
    c_right<T0, inner_magic0, T1, inner_magic1>::store(tl_object, std::move(X), std::move(Y));
  } else {
    CurrentProcessingQuery::get().raise_storing_error("Invalid constructor %s of type %s", c_name.c_str(), "Either");
  }
}
 * Типизированно:
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
    CurrentProcessingQuery::get().raise_storing_error("Invalid constructor %s of type %s", tl_object.get_class(), "Either");
  }
}
*/
struct TypeStore {
  const vk::tl::type *type;
  std::string template_str;
  bool typed_mode;

  TypeStore(const vk::tl::type *type, string template_str, bool typed_mode = false) :
    type(type),
    template_str(std::move(template_str)),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const;
};


// Пример сгенеренного кода:
/*
 * Нетипизированно:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
array<var> t_Either<T0, inner_magic0, T1, inner_magic1>::fetch() {
  array<var> result;
  CHECK_EXCEPTION(return result);
  auto magic = static_cast<unsigned int>(f$fetch_int());
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
      CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Either: 0x%08x", magic);
    }
  }
  return result;
}
 * Типизированно:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::typed_fetch_to(PhpType &tl_object) {
  CHECK_EXCEPTION(return);
  auto magic = static_cast<unsigned int>(f$fetch_int());
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
      CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Either: 0x%08x", magic);
    }
  }
}
*/
struct TypeFetch {
  const vk::tl::type *type;
  std::string template_str;
  bool typed_mode;

  inline TypeFetch(const vk::tl::type *type, string template_str, bool typed_mode = false) :
    type(type),
    template_str(std::move(template_str)),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const;
};


/*
 * Пример типизированного:
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

  void store(const var& tl_object);
  array<var> fetch();
  void typed_store(const PhpType &tl_object);
  void typed_fetch_to(PhpType &tl_object);
};
*/
struct TlTypeDeclaration {
  const vk::tl::type *t;

  // tl-типу 'messages.ChatInfoUser' соответствует:
  // * либо php class VK\TL\Types\messages\chatInfoUser
  // * либо php interface VK\TL\Types\messages\ChatInfoUser, если тип полиморфный
  static bool does_tl_type_need_typed_fetch_store(const vk::tl::type *t) {
    if (t->name == "ReqResult") {
      // без этого сайт не компилится, пока typed rpc не подключен — т.к. типизированный t_ReqResult не компилится
      bool typed_php_code_exists = !!G->get_class(vk::tl::PhpClasses::rpc_response_ok_with_tl_full_namespace());
      return typed_php_code_exists;
    }
    return !get_all_php_classes_of_tl_type(t).empty();
  }

  explicit TlTypeDeclaration(const vk::tl::type *t) :
    t(t) {}

  void compile(CodeGenerator &W) const;
};

struct TlTypeDefinition {
  const vk::tl::type *t;

  explicit TlTypeDefinition(const vk::tl::type *t) :
    t(t) {}

  void compile(CodeGenerator &W) const;
};
}
