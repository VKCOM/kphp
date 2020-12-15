// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vk {
namespace tlo_parsing {

constexpr int NODE_TYPE_TYPE = 1;
constexpr int NODE_TYPE_NAT_CONST = 2;
constexpr int NODE_TYPE_VAR_TYPE = 3;
constexpr int NODE_TYPE_VAR_NUM = 4;
constexpr int NODE_TYPE_ARRAY = 5;
constexpr int FLAG_BARE = (1 << 0);
constexpr int FLAG_NOCONS = (1 << 1);
constexpr int FLAG_OPT_VAR = (1 << 17);
constexpr int FLAG_EXCL = (1 << 18);
constexpr int FLAG_OPT_FIELD = (1 << 20);
constexpr int FLAG_NOVAR = (1 << 21);
constexpr int FLAG_DEFAULT_CONSTRUCTOR = (1 << 25);
constexpr int FLAGS_MASK = ((1 << 16) - 1);

constexpr int COMBINATOR_FLAG_READ = 1;
constexpr int COMBINATOR_FLAG_WRITE = 2;
constexpr int COMBINATOR_FLAG_INTERNAL = 4;
constexpr int COMBINATOR_FLAG_KPHP = 8;

constexpr unsigned int TL_TRUE_ID = 0x3fedd339U;  // True
constexpr unsigned int TL_TYPE_ID = 0x2cecf817U;  // Type
constexpr unsigned int TL_SHARP_ID = 0x70659effU; // #
constexpr unsigned int TL_MAYBE_ID = 0x180f8483U; // Maybe

struct tl_scheme;
struct type;
struct combinator;
struct arg;
struct expr_base;
struct nat_expr_base;
struct type_expr_base;
struct type_expr;
struct type_var;
struct type_array;
struct nat_const;
struct nat_var;

struct expr_visitor {
  virtual void apply(type_expr &) = 0;
  virtual void apply(type_var &) = 0;
  virtual void apply(type_array &) = 0;
  virtual void apply(nat_const &) = 0;
  virtual void apply(nat_var &) = 0;
  virtual ~expr_visitor() = default;
};

struct const_expr_visitor {
  virtual void apply(const type_expr &) = 0;
  virtual void apply(const type_var &) = 0;
  virtual void apply(const type_array &) = 0;
  virtual void apply(const nat_const &) = 0;
  virtual void apply(const nat_var &) = 0;
  virtual ~const_expr_visitor() = default;
};

struct expr_base {
  int flags;

  template <typename T>
  T *as() { return dynamic_cast<T *>(this); }

  template<typename T>
  const T *as() const { return dynamic_cast<const T *>(this); }

  bool is_bare() const;
  virtual std::string to_str() const = 0;
  virtual ~expr_base() = default;

  virtual void visit(expr_visitor &visitor) = 0;
  virtual void visit(const_expr_visitor &visitor) const = 0;
};

struct nat_expr_base : expr_base {
};

struct type_expr_base : expr_base {
};

struct tlo_parser;

struct type_expr final : type_expr_base {
  int type_id = {};
  std::vector<std::unique_ptr<expr_base>> children = {};

  type_expr() = default;
  explicit type_expr(int type_id, std::vector<std::unique_ptr<expr_base>> children = {});
  explicit type_expr(tlo_parser *reader);

  std::string to_str() const final;

  void visit(expr_visitor &visitor) final { visitor.apply(*this); };
  void visit(const_expr_visitor &visitor) const final { visitor.apply(*this); };
};

struct type_var final : type_expr_base {
  // от какого var_num зависит (используется в дереве)
  // например:
  //   {t: Type} x:t
  //   тут x - аргумент, у которого
  //     type_expr имеет тип type_var,
  //       var_num которого, указывает на {t: Type}
  int var_num = {};
  combinator *owner{};

  type_var() = default;
  explicit type_var(int var_num);
  explicit type_var(tlo_parser *reader);

  std::string get_name() const;

  std::string to_str() const final;

  void visit(expr_visitor &visitor) final { visitor.apply(*this); };
  void visit(const_expr_visitor &visitor) const final { visitor.apply(*this); };
};

struct arg {
  int flags = {};
  int idx = {};
  int var_num = {}; // когда от этого аргумента, кто-то зависит
  int exist_var_num = {}; // от какого var_num зависит этот агрумент (только в случае с филд масками)
  int exist_var_bit = {}; // от какого бита этого var_num зависит (только в случае с филд масками)
  std::unique_ptr<type_expr_base> type_expr = {};
  std::string name = {};

  arg() = default;

  arg(tlo_parser *reader, int idx);

  bool is_type() const;
  bool is_sharp() const;
  bool is_named_fields_mask_bit() const;
  bool is_fields_mask_optional() const;
  bool is_optional() const;
  bool is_forwarded_function() const;
  std::string to_str() const;
};


struct type_array final : type_expr_base {
  int cell_len = {};
  std::unique_ptr<nat_expr_base> multiplicity = {};
  std::vector<std::unique_ptr<arg>> args = {};

  type_array() = default;
  type_array(int cell_len, std::unique_ptr<nat_expr_base> multiplicity, std::vector<std::unique_ptr<arg>> args);
  explicit type_array(tlo_parser *reader);

  std::string to_str() const final;

  void visit(expr_visitor &visitor) final { visitor.apply(*this); };
  void visit(const_expr_visitor &visitor) const final { visitor.apply(*this); };
};

struct nat_const final : nat_expr_base {
  int num = {}; // само значение

  nat_const() = default;
  explicit nat_const(tlo_parser *reader);

  std::string to_str() const final;

  void visit(expr_visitor &visitor) final { visitor.apply(*this); };
  void visit(const_expr_visitor &visitor) const final { visitor.apply(*this); };
};

struct nat_var final : nat_expr_base {
  int diff = {};
  int var_num = {}; // аналогично type_var, но про #
  combinator *owner{};

  nat_var() = default;

  explicit nat_var(tlo_parser *reader);

  std::string get_name() const;
  std::string to_str() const final;

  void visit(expr_visitor &visitor) final { visitor.apply(*this); };
  void visit(const_expr_visitor &visitor) const final { visitor.apply(*this); };
};


struct combinator {
  enum class combinator_type {
    CONSTRUCTOR,
    FUNCTION
  };

  combinator_type kind = {};
  int flags = {};
  int id = {};
  int type_id = {};
  std::vector<std::unique_ptr<arg>> args = {};
  // в случае flat оптимизации, сюда будет записан magic id оригинального конструктора result
  int original_result_constructor_id = {};
  std::unique_ptr<type_expr_base> result = {};
  std::string name = {};

  combinator() = default;

  explicit combinator(tlo_parser *reader, combinator_type kind);

  arg *get_var_num_arg(int var_num) const;
  size_t get_type_parameter_input_index(int var_num) const;
  bool is_function() const;
  bool is_constructor() const;
  bool is_read_function() const;
  bool is_write_function() const;
  bool is_internal_function() const;
  bool is_kphp_rpc_server_function() const;
  bool is_generic() const;
  bool is_dependent() const;
  bool has_fields_mask() const;
  std::string to_str() const;
private:
  std::vector<std::pair<int, int>> var_num_to_arg_idx = {};
};


struct type {
  int id = {};
  int flags = {};
  int constructors_num = {};
  int arity = {};
  std::vector<std::unique_ptr<combinator>> constructors = {};
  std::string name = {};

  type() = default;

  explicit type(tlo_parser *reader);

  bool is_polymorphic() const;
  bool is_integer_variable() const;
  bool is_generic() const;
  bool is_dependent() const;
  bool has_fields_mask() const;
  bool is_builtin() const;
  std::string to_str() const;
};


struct tl_scheme {
  int scheme_version = {};
  std::unordered_map<int, std::unique_ptr<type>> types;
  std::unordered_map<int, std::unique_ptr<combinator>> functions;
  std::unordered_map<std::string, int> magics;      // tl type or function name => magic
  std::unordered_map<int, int> owner_type_magics;   // constructor magic        => magic of its owner type

  vk::tlo_parsing::combinator *get_constructor_by_magic(int magic) const;
  vk::tlo_parsing::type *get_type_by_magic(int magic) const;
  vk::tlo_parsing::combinator *get_function_by_magic(int magic) const;
  std::string to_str() const;

  void remove_type(const type *&t);
  void remove_function(const combinator *&f);
};

inline vk::tlo_parsing::type *get_type_of(const type_expr *expr, const vk::tlo_parsing::tl_scheme *scheme) {
  vk::tlo_parsing::type *res = scheme->get_type_by_magic(expr->type_id);
  assert(res);
  return res;
}

inline vk::tlo_parsing::type *get_type_of(const combinator *constructor, const vk::tlo_parsing::tl_scheme *scheme) {
  assert(constructor->is_constructor());
  vk::tlo_parsing::type *res = scheme->get_type_by_magic(constructor->type_id);
  assert(res);
  return res;
}

} // namespace tlo_parsing
} // namespace vk
