// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tlo-parsing/tl-objects.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>

#include "common/algorithms/find.h"
#include "common/tl/tls.h"
#include "common/tlo-parsing/tlo-parser.h"

namespace vk {
namespace tlo_parsing {
static combinator *cur_parsed_combinator;

type_expr::type_expr(tlo_parser *reader) {
  type_id = reader->get_value<int>();
  const std::unique_ptr<type> &t = reader->tl_sch->types[type_id];
  flags = reader->get_value<int>() | FLAG_NOVAR;
  int arity = reader->get_value<int>();
  if (t->arity != arity) {
    reader->error("Unexpected arity of type_expr: expected %d, but was %d", t->arity, arity);
    return;
  }
  for (int i = 0; i < t->arity; ++i) {
    children.push_back(reader->read_expr());
    if (!(children.back()->flags & FLAG_NOVAR)) {
      flags &= ~FLAG_NOVAR;
    }
  }
}

type_var::type_var(tlo_parser *reader) {
  owner = cur_parsed_combinator;
  var_num = reader->get_value<int>();
  flags = reader->get_value<int>();
  if (flags & (FLAG_NOVAR | FLAG_BARE)) {
    reader->error("Incorrect flags of type_var: FLAG_NOVAR and FLAG_BARE are expected not to be set\ntype-var:\n%s", to_str().c_str());
  }
}

arg::arg(tlo_parser *reader, int idx) :
  idx(idx) {
  int schema_flag_opt_field;
  int schema_flag_has_vars;
  if (reader->tl_sch->scheme_version >= 3) {
    schema_flag_opt_field = 4;
    schema_flag_has_vars = 2;
  } else {
    schema_flag_opt_field = 2;
    schema_flag_has_vars = 4;
  }
  auto arg_v = reader->get_value<unsigned int>();
  if (arg_v != TL_TLS_ARG) {
    reader->error("Unexpected tls_arg_v2: expected %08x, but was %08x", TL_TLS_ARG, arg_v);
    return;
  }

  name = reader->get_string();
  flags = reader->get_value<int>();
  if (flags & schema_flag_opt_field) {
    flags &= ~schema_flag_opt_field;
    flags |= FLAG_OPT_FIELD;
  }
  if (flags & schema_flag_has_vars) {
    flags &= ~schema_flag_has_vars;
    var_num = reader->get_value<int>();
  } else {
    var_num = -1;
  }
  if (flags & FLAG_OPT_FIELD) {
    exist_var_num = reader->get_value<int>();
    exist_var_bit = reader->get_value<int>();
  } else {
    exist_var_num = -1;
    exist_var_bit = 0;
  }
  type_expr = reader->read_type_expr();
  if (var_num < 0 && exist_var_num < 0 && (type_expr->flags & FLAG_NOVAR)) {
    flags |= FLAG_NOVAR;
  }
}

type_array::type_array(tlo_parser *reader) {
  multiplicity = reader->read_nat_expr();
  cell_len = reader->get_value<int>();
  args.reserve(cell_len);
  for (int i = 0; i < cell_len; ++i) {
    args.emplace_back(std::make_unique<arg>(reader, i + 1));
  }
  flags = FLAG_NOVAR;
  for (const auto &arg: args) {
    if (!(arg->flags & FLAG_NOVAR)) {
      flags &= ~FLAG_NOVAR;
      break;
    }
  }
}

nat_const::nat_const(tlo_parser *reader) {
  num = reader->get_value<int>();
  flags = FLAG_NOVAR;
}

nat_var::nat_var(tlo_parser *reader) {
  owner = cur_parsed_combinator;
  diff = reader->get_value<int>();
  var_num = reader->get_value<int>();
  flags = 0;
}

combinator::combinator(tlo_parser *reader, combinator_type kind) :
  kind(kind) {
  cur_parsed_combinator = this;
  id = reader->get_value<int>();
  name = reader->get_string();
  type_id = reader->get_value<int>();
  auto left_type = reader->get_value<unsigned int>();
  if (left_type == TL_TLS_COMBINATOR_LEFT) {
    auto num = reader->get_value<unsigned int>();
    args.reserve(num);
    for (int i = 0; i < num; ++i) {
      args.emplace_back(std::make_unique<arg>(reader, i + 1));
    }
  } else {
    if (left_type != TL_TLS_COMBINATOR_LEFT_BUILTIN) {
      reader->error("Error while parsing combinator %s: unexpected left_type\nExpected %08x, but was %08x",
                    name.c_str(), TL_TLS_COMBINATOR_LEFT_BUILTIN, left_type);
      return;
    }
  }
  auto tls_ctor_right_v2 = reader->get_value<unsigned int>();
  if (tls_ctor_right_v2 != TL_TLS_COMBINATOR_RIGHT) {
    reader->error("Error while parsing combinator %s: unexpected tls_combinator_right_v\nExpected %08x, but was %08x",
                  name.c_str(), TL_TLS_COMBINATOR_RIGHT, tls_ctor_right_v2);
    return;
  }
  result = reader->read_type_expr();
  for (const auto &arg: args) {
    if (arg->var_num != -1) {
      var_num_to_arg_idx.emplace_back(arg->var_num, arg->idx - 1);
    }
  }
  if (reader->tl_sch->scheme_version >= 4) {
    flags = reader->get_value<int>();
  }
}

arg *combinator::get_var_num_arg(int var_num) const {
  for (const auto &e : var_num_to_arg_idx) {
    if (e.first == var_num) {
      return args[e.second].get();
    }
  }
  return nullptr;
}

size_t combinator::get_type_parameter_input_index(int var_num) const {
  const auto *template_args = result->as<type_expr>();
  assert(template_args);
  size_t template_arg_number = 0;
  for (const auto &child: template_args->children) {
    const auto *child_type_var = child->as<type_var>();
    if (child_type_var && child_type_var->var_num == var_num) {
      break;
    }
    const auto *child_nat_var = child->as<nat_var>();
    if (child_nat_var && child_nat_var->var_num == var_num) {
      break;
    }
    ++template_arg_number;
  }

  assert(template_arg_number != template_args->children.size());
  return template_arg_number;
}

bool combinator::is_function() const {
  return kind == combinator_type::FUNCTION;
}

bool combinator::is_constructor() const {
  return kind == combinator_type::CONSTRUCTOR;
}

bool combinator::is_read_function() const {
  assert(is_function());
  return flags & COMBINATOR_FLAG_READ;
}

bool combinator::is_write_function() const {
  assert(is_function());
  return flags & COMBINATOR_FLAG_WRITE;
}

bool combinator::is_internal_function() const {
  assert(is_function());
  return flags & COMBINATOR_FLAG_INTERNAL;
}

bool combinator::is_kphp_rpc_server_function() const {
  assert(is_function());
  return flags & COMBINATOR_FLAG_KPHP;
}

static void verify_hardcoded_magics(unsigned int id, const std::string &name) {
  static const std::map<std::string, unsigned int> hardcoded_magics = {{"#",     TL_SHARP_ID},
                                                                       {"Type",  TL_TYPE_ID},
                                                                       {"True",  TL_TRUE_ID},
                                                                       {"Maybe", TL_MAYBE_ID}};
  auto it = hardcoded_magics.find(name);
  if (it != hardcoded_magics.end()) {
    if (id != it->second) {
      fprintf(stderr, "FATAL ERROR!\nFor type %s hardcoded magic: %08x, but actual magic: %08x",
              name.c_str(), it->second, id);
      assert(false);
    }
  }
}

type::type(tlo_parser *reader) {
  id = reader->get_value<int>();
  name = reader->get_string();
  constructors_num = reader->get_value<int>();
  flags = reader->get_value<int>();
  if (name == "Maybe" || name == "Bool") {
    flags |= FLAG_NOCONS;
  }
  arity = reader->get_value<int>();
  reader->get_value<int64_t>();  // probably, should use it
  verify_hardcoded_magics(id, name);
}

bool arg::is_type() const {
  if (auto *casted = type_expr->as<vk::tlo_parsing::type_expr>()) {
    if (casted->type_id == TL_TYPE_ID) {
      return true;
    }
  }
  return false;
}

bool arg::is_sharp() const {
  if (auto *casted = type_expr->as<vk::tlo_parsing::type_expr>()) {
    if (casted->type_id == TL_SHARP_ID) {
      return true;
    }
  }
  return false;
}

static inline std::string to_hex_str(unsigned int x) {
  const size_t buf_size = 16;
  char buf[buf_size];
  snprintf(buf, buf_size, "%08x", x);
  return {buf, 8};
}

vk::tlo_parsing::combinator *tl_scheme::get_constructor_by_magic(int magic) const {
  auto magic_iter = owner_type_magics.find(magic);
  if (magic_iter == owner_type_magics.end()) {
    return nullptr;
  }
  auto owner_type_iter = types.find(magic_iter->second);
  if (owner_type_iter == types.end()) {
    return nullptr;
  }
  for (const auto &e : owner_type_iter->second->constructors) {
    if (e->id == magic) {
      return e.get();
    }
  }
  return nullptr;
}

std::string flags_to_str(int v) {
  constexpr std::pair<int, const char *> available_flags[] = {
    {FLAG_BARE,                "FLAG_BARE"},
    {FLAG_NOCONS,              "FLAG_NOCONS"},
    {FLAG_OPT_VAR,             "FLAG_OPT_VAR"},
    {FLAG_EXCL,                "FLAG_EXCL"},
    {FLAG_OPT_FIELD,           "FLAG_OPT_FIELD"},
    {FLAG_NOVAR,               "FLAG_NOVAR"},
    {FLAG_DEFAULT_CONSTRUCTOR, "FLAG_DEFAULT_CONSTRUCTOR"}
  };

  std::string result;
  for (const auto &flag : available_flags) {
    if (v & flag.first) {
      result.append(flag.second).append("|");
      v ^= flag.first;
    }
  }

  if (v) {
    result.append("UNKNOWN FLAGS {").append(to_hex_str(v)).append("}|");
  }
  if (result.empty()) {
    result.assign("NO FLAGS");
  } else {
    result.pop_back();
  }

  return result;
}

std::string tl_scheme::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "TL-SCHEME:\n";
  ss << "scheme-version: " << scheme_version << "\n";
  ss << "types:\n";
  for (const auto &t: types) {
    ss << t.second->to_str();
  }
  ss << "functions:\n";
  for (const auto &f: functions) {
    ss << f.second->to_str();
  }
  return ss.str();
}

vk::tlo_parsing::type *tl_scheme::get_type_by_magic(int magic) const {
  auto type_iter = types.find(magic);
  if (type_iter == types.end()) {
    return nullptr;
  }
  return type_iter->second.get();
}

vk::tlo_parsing::combinator *vk::tlo_parsing::tl_scheme::get_function_by_magic(int magic) const {
  auto function_iter = functions.find(magic);
  if (function_iter == functions.end()) {
    return nullptr;
  }
  return function_iter->second.get();
}

bool type::is_polymorphic() const {
  assert(!constructors.empty());
  // тип полиморфный <=> это php interface, у которого php class-конструкторы, если
  return constructors.size() > 1 ||                                     // либо несколько конструкторов
         constructors.front()->name.size() != name.size() ||            // либо один, но его название отличается
         strcasecmp(constructors.front()->name.c_str(), name.c_str());  // (значит, планируется добавление других в будущем)
}

bool type::is_integer_variable() const {
  return name == "#";
}

std::string type::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "type: " << name << "\n" <<
     "\tid: " << to_hex_str(id) << "\n" <<
     "\tarity: " << arity << "\n" <<
     "\tflags: " << flags_to_str(flags) << "\n" <<
     "constructors:\n";
  for (const auto &c: constructors) {
    ss << c->to_str();
  }
  return ss.str();
}

bool type::is_generic() const {
  if (constructors.empty()) {
    return false;
  }
  return constructors.front()->is_generic();
}

bool type::is_dependent() const {
  if (constructors.empty()) {
    return false;
  }
  return constructors.front()->is_dependent();
}

bool type::is_builtin() const {
  return vk::any_of_equal(name, "#", "Type", "Int", "Long", "Float", "Double", "String");
}

bool type::has_fields_mask() const {
  return std::all_of(constructors.begin(), constructors.end(), [](const std::unique_ptr<combinator> & c) {
    return c->has_fields_mask();
  });
}

std::string combinator::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "combinator: " << name << "\n" <<
     "\tid: " << to_hex_str(id) << "\n" <<
     "\tkind: " << (kind == combinator::combinator_type::FUNCTION ? "function" : "constructor") << "\n" <<
     "\ttype_id: " << to_hex_str(type_id) << "\n" <<
     "args:\n";
  for (const auto &arg: args) {
    ss << arg->to_str();
  }
  ss << "result_type:\n" << result->to_str();
  return ss.str();
}

bool combinator::is_generic() const {
  return std::any_of(args.begin(), args.end(), [](const std::unique_ptr<arg> &arg) {
    return arg->is_type();
  });
}

bool combinator::is_dependent() const {
  return std::any_of(args.begin(), args.end(), [](const std::unique_ptr<arg> &arg) {
    return arg->is_optional() && arg->is_sharp();
  });
}

bool combinator::has_fields_mask() const {
  return std::any_of(args.begin(), args.end(), [](const std::unique_ptr<arg> &arg) {
    return arg->is_fields_mask_optional();
  });
}

std::string nat_var::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "diff: " << diff << "\n" <<
     "var_num: " << var_num << "\n" <<
     "flags: " << flags_to_str(flags) << "\n";
  return ss.str();
}

std::string nat_var::get_name() const {
  return owner->get_var_num_arg(var_num)->name;
}

std::string nat_const::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "value: " << num << "\n" <<
     "flags: " << flags_to_str(flags) << "\n";
  return ss.str();
}

std::string type_array::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "tl-type-array:\n";
  ss << "\tflags: " << flags_to_str(flags) << "\n";
  ss << "\tcell_len: " << cell_len << "\n";
  ss << "\tmultiplicity: " << multiplicity->to_str() << "\n";
  ss << "args:\n";
  for (const auto &arg: args) {
    ss << arg->to_str();
  }
  return ss.str();
}
type_array::type_array(int cell_len, std::unique_ptr<nat_expr_base> multiplicity, std::vector<std::unique_ptr<arg>> args) :
  cell_len(cell_len),
  multiplicity(std::move(multiplicity)),
  args(std::move(args)) {}

std::string arg::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "arg #" << idx << ": \n";
  ss << "\tname: " << name << "\n";
  ss << "\tflags: " << flags_to_str(flags) << "\n";
  ss << "\tvar_num: " << var_num << "\n";
  ss << "\texist_var_num: " << exist_var_num << "\n";
  ss << "\texist_var_bit: " << exist_var_bit << "\n";
  ss << "type_expr:\n";
  ss << type_expr->to_str();
  return ss.str();
}

bool arg::is_fields_mask_optional() const {
  return flags & FLAG_OPT_FIELD;
}

bool arg::is_optional() const {
  return flags & FLAG_OPT_VAR;
}

bool arg::is_forwarded_function() const {
  return flags & FLAG_EXCL;
}

bool arg::is_named_fields_mask_bit() const {
  if (auto *expr = type_expr->as<vk::tlo_parsing::type_expr>()) {
    return is_fields_mask_optional() && expr->type_id == TL_TRUE_ID && expr->is_bare();
  }
  return false;
}

std::string type_var::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "type-var\n";
  ss << "\tflags: " << flags_to_str(flags) << "\n";
  ss << "\tvar_num: " << var_num << "\n";
  return ss.str();
}

type_var::type_var(int var_num) :
  var_num(var_num) {}

std::string type_var::get_name() const {
  return owner->get_var_num_arg(var_num)->name;
}

std::string type_expr::to_str() const {
  std::stringstream ss;
  ss.width(8);
  ss << "type-expr\n";
  ss << "\tflags: " << flags_to_str(flags) << "\n";
  ss << "\ttype_id: " << to_hex_str(type_id) << "\n";
  ss << "\ttype_expr:\n";
  for (const auto &child: children) {
    ss << child->to_str() << "\n";
  }
  return ss.str();
}

type_expr::type_expr(int type_id, std::vector<std::unique_ptr<expr_base>> children) :
  type_id(type_id),
  children(std::move(children)) {}

bool expr_base::is_bare() const {
  return flags & FLAG_BARE;
}

void tl_scheme::remove_type(const type *&t) {
  for (const auto &c : t->constructors) {
    owner_type_magics.erase(c->id);
  }
  magics.erase(t->name);
  types.erase(t->id);
  t = nullptr; // destroyed after erasing
}

void tl_scheme::remove_function(const combinator *&f) {
  magics.erase(f->name);
  functions.erase(f->id);
  f = nullptr; // destroyed after erasing
}

} // namespace tlo_parsing
} // namespace vk
