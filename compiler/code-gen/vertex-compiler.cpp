// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/vertex-compiler.h"

#include <iterator>
#include <unordered_map>

#include "common/wrappers/field_getter.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"
#include "compiler/vertex.h"

struct Operand {
  VertexPtr root;
  Operation parent_type;
  bool is_left;

  void compile(CodeGenerator &W) const {
    int priority = OpInfo::priority(parent_type);
    bool left_to_right = OpInfo::fixity(parent_type) == left_opp;

    int root_priority = OpInfo::priority(root->type());

    bool need_par = (root_priority < priority || (root_priority == priority && (left_to_right ^ is_left))) && root_priority > 0;
    need_par |= parent_type == op_log_and_let || parent_type == op_log_or_let || parent_type == op_log_xor_let;

    if (need_par) {
      W << "(";
    }

    W << root;

    if (need_par) {
      W << ")";
    }
  }
};

struct TupleGetIndex {
  VertexPtr tuple;
  std::string int_index;

  TupleGetIndex(VertexPtr tuple, std::string int_index) :
    tuple(tuple),
    int_index(std::move(int_index)) {
  }

  TupleGetIndex(VertexPtr tuple, VertexPtr key) :
    tuple(tuple),
    int_index(GenTree::get_actual_value(key)->get_string()) {
  }

  void compile(CodeGenerator &W) const {
    W << "std::get<" << int_index << ">(" << tuple << ")";
  }
};

struct ShapeGetIndex {
  VertexPtr shape;
  std::string index;

  ShapeGetIndex(VertexPtr shape, std::string index) :
    shape(shape),
    index(std::move(index)) {
  }

  ShapeGetIndex(VertexPtr shape, VertexPtr key) :
    shape(shape),
    index(GenTree::get_actual_value(key)->get_string()) {
  }

  void compile(CodeGenerator &W) const {
    W << shape << ".get<" << static_cast<size_t>(string_hash(index.c_str(), index.size())) << "UL>()";
  }
};

struct AsSeq {
  VertexAdaptor<op_seq> root;

  void compile(CodeGenerator &W) const {
    for (auto i : *root) {
      if (vk::none_of_equal(i->type(), op_var, op_empty)) {
        W << i << ";" << NL;
      }
    }
  }
};

struct Label {
  int label_id;

  void compile(CodeGenerator &W) const {
    if (label_id != 0) {
      W << NL << LabelName{label_id} << ":;" << NL;
    }
  }
};

struct CycleBody {
  VertexAdaptor<op_seq> body;
  int continue_label_id;
  int break_label_id;

  void compile(CodeGenerator &W) const {
    W << BEGIN <<
      AsSeq{body} <<
      Label{continue_label_id} <<
      END <<
      Label{break_label_id};
  }
};

void compile_prefix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  W << OpInfo::str(root->type()) << Operand{root->expr(), root->type(), true};
}

void compile_postfix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  W << Operand{root->expr(), root->type(), true} << OpInfo::str(root->type());
}

void compile_conv_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  if (root->type() == op_conv_regexp) {
    W << root->expr();
  } else {
    W << OpInfo::str(root->type()) << " (" << root->expr() << ")";
  }
  if (vk::any_of_equal(root->type(), op_conv_drop_null, op_conv_drop_false)) {
    if (tinf::get_type(root->expr())->use_optional() && !tinf::get_type(root)->use_optional()) {
      W << ".val()";
    }
  }
}

void compile_noerr(VertexAdaptor<op_noerr> root, CodeGenerator &W) {
  if (root->rl_type == val_none) {
    W << "NOERR_VOID" << MacroBegin{} << Operand{root->expr(), root->type(), true} << MacroEnd{};
  } else {
    const TypeData *res_tp = tinf::get_type(root);
    W << "NOERR" << MacroBegin{} << Operand{root->expr(), root->type(), true} << ", " << TypeName(res_tp) << MacroEnd{};
  }
}

//TODO: some interface for context?
//TODO: it's copypasted to compile_return
void compile_throw_action(CodeGenerator &W) {
  CGContext &context = W.get_context();
  if (context.catch_labels.empty() || context.catch_labels.back().empty()) {
    const TypeData *tp = tinf::get_type(context.parent_func, -1);
    if (context.resumable_flag) {
      kphp_assert(!context.inside_null_coalesce_fallback);
      if (tp->ptype() != tp_void) {
        W << "RETURN (";
      } else {
        W << "RETURN_VOID (";
      }
    } else {
      W << "return ";
    }
    if (context.inside_null_coalesce_fallback || tp->ptype() != tp_void) {
      W << "{}";
    }
    if (context.resumable_flag) {
      W << ")";
    }
  } else {
    W << "goto " << context.catch_labels.back();
    context.catch_label_used.back() = 1;
  }
}

void compile_throw(VertexAdaptor<op_throw> root, CodeGenerator &W) {
  W << BEGIN <<
    "THROW_EXCEPTION " << MacroBegin{} << root->exception() << MacroEnd{} << ";" << NL;
  compile_throw_action(W);
  W << ";" << NL <<
    END << NL;
}

void compile_try(VertexAdaptor<op_try> root, CodeGenerator &W) {
  CGContext &context = W.get_context();

  string catch_label = gen_unique_name("catch_label");
  W << "/""*** TRY ***""/" << NL;
  context.catch_labels.push_back(catch_label);
  context.catch_label_used.push_back(0);
  W << root->try_cmd() << NL;
  context.catch_labels.pop_back();
  bool used = context.catch_label_used.back();
  context.catch_label_used.pop_back();

  if (used) {
    W << "/""*** CATCH ***""/" << NL <<
      "if (0) " <<
      BEGIN <<
      catch_label << ":;" << NL << //TODO: Label (lable_id) ?
      root->exception() << " = std::move(CurException);" << NL <<
      root->catch_cmd() << NL <<
      END << NL;
  }
}

void compile_power(VertexAdaptor<op_pow> power, CodeGenerator &W) {
  switch (tinf::get_type(power)->ptype()) {
    case tp_int:
      // pow return type with positive constexpr integer exponent and any integer base is inferred as int
      W << "int_power";
      return;
    case tp_float:
      // pow return type with positive constexpr integer exponent and any float base is inferred as float
      W << "float_power";
      return;
    case tp_mixed:
      // pow return type with any other types in exponent,
      // including negative constexpr integer or unknown integer, is inferred as var
      W << "var_power";
      return;
    default:
      kphp_error(false, "Unexpected power return type");
  }
}

/**
 * If array is indexed by a constant string like in $a['somekey'], we want to precompute
 * the string key hash at the compile time so we don't have to do it during the run time.
 * This method checks whether a key is a constant key and if it is, computes the hash.
 * @return int string_hash or 0 (if string hash itself is 0, this array access won't be optimized)
 */
inline int64_t can_use_precomputed_hash_indexing_array(VertexPtr key) {
  // if it's just a ['string'] that became [$const_string$xxx] (it can also be op_concat or other kind thing)
  if (auto key_var = key.try_as<op_var>()) {
    if (key->extra_type == op_ex_var_const && key_var->var_id->init_val->type() == op_string) {
      const std::string &string_key = key_var->var_id->init_val->get_string();

      // see array::get_value()/set_value(): numeric strings have a separate code path
      int64_t int_val;
      if (php_try_to_int(string_key.c_str(), string_key.size(), &int_val)) {
        return 0;
      }

      return string_hash(string_key.c_str(), string_key.size());
    }
  }

  return 0;
}

void compile_null_coalesce(VertexAdaptor<op_null_coalesce> root, CodeGenerator &W) {
  const TypeData *type = tinf::get_type(root);
  auto lhs = root->lhs();
  auto rhs = root->rhs();

  // rhs is wrapped into lambda, therefore we need special call for exception throwing cases
  if (rhs->throw_flag) {
    W << "TRY_CALL_ " << MacroBegin{} << TypeName{type} << ", ";
  }
  W << "null_coalesce< " << TypeName{type} << " >(";
  const auto index = lhs.try_as<op_index>();
  const auto array_ptype = index ? tinf::get_type(index->array())->get_real_ptype() : tp_Unknown;
  if (index && vk::none_of_equal(array_ptype, tp_shape, tp_tuple)) {
    kphp_assert (index->has_key());
    W << index->array() << ", " << index->key() << ", ";
    if (vk::any_of_equal(array_ptype, tp_array, tp_mixed)) {
      if (auto precomputed_hash = can_use_precomputed_hash_indexing_array(index->key())) {
        W << precomputed_hash << "L, ";
      }
    }
  } else {
    W << lhs << ", ";
  }

  if (vk::any_of_equal(rhs->type(), op_var, op_int_const, op_float_const, op_false, op_null)) {
    W << rhs;
  } else {
    auto &context = W.get_context();
    context.catch_labels.emplace_back();
    ++context.inside_null_coalesce_fallback;
    FunctionSignatureGenerator(W) << "[&] ()"; W << " -> " << TypeName{tinf::get_type(rhs)} << " " << BEGIN
                                  << " return " << rhs << ";" << NL
                                  << END;
    context.catch_labels.pop_back();
    kphp_assert(context.inside_null_coalesce_fallback > 0);
    context.inside_null_coalesce_fallback--;
  }

  W << ")";
  if (rhs->throw_flag) {
    W << ", ";
    compile_throw_action(W);
    W << MacroEnd{};
  }
}

void compile_binary_func_op(VertexAdaptor<meta_op_binary> root, CodeGenerator &W) {
  if (auto pow_vertex = root.try_as<op_pow>()) {
    compile_power(pow_vertex, W);
  } else {
    W << OpInfo::str(root->type());
  }
  W << " (" <<
    Operand{root->lhs(), root->type(), true} <<
    ", " <<
    Operand{root->rhs(), root->type(), false} <<
    ")";
}


void compile_binary_op(VertexAdaptor<meta_op_binary> root, CodeGenerator &W) {
  auto &root_type_str = OpInfo::str(root->type());
  kphp_error_return (root_type_str[0] != '@', fmt_format("Unexpected {}\n", vk::string_view{root_type_str}.substr(1)));

  auto lhs = root->lhs();
  auto rhs = root->rhs();

  auto lhs_tp = tinf::get_type(lhs);
  auto rhs_tp = tinf::get_type(rhs);

  if (auto instanceof = root.try_as<op_instanceof>()) {
    W << "f$is_a<" << instanceof->derived_class->src_name << ">(" << lhs << ")";
    return;
  }

  if (auto null_coalesce_vertex = root.try_as<op_null_coalesce>()) {
    compile_null_coalesce(null_coalesce_vertex, W);
    return;
  }

  if (root->type() == op_add) {
    if (lhs_tp->ptype() == tp_array && rhs_tp->ptype() == tp_array && type_out(lhs_tp) != type_out(rhs_tp)) {
      const TypeData *res_tp = tinf::get_type(root)->const_read_at(Key::any_key());
      W << "array_add < " << TypeName(res_tp) << " > (" << lhs << ", " << rhs << ")";
      return;
    }
  }
  // special inplace variables that are defined at the assignment location, not in the beginning of the function
  if (root->type() == op_set && lhs->type() == op_var && lhs->extra_type == op_ex_var_superlocal_inplace) {
    W << TypeName(lhs_tp) << " ";    // generates "array<T> $tmp = v" instead of "$tmp = v"
  }

  if (OpInfo::type(root->type()) == binary_func_op) {
    compile_binary_func_op(root, W);
    return;
  }

  bool lhs_is_bool = lhs_tp->get_real_ptype() == tp_bool;
  bool rhs_is_bool = rhs_tp->get_real_ptype() == tp_bool;
  if (rhs_is_bool ^ lhs_is_bool) {
    static const std::unordered_map<int, const char *, std::hash<int>> tp_to_str{
      {op_lt,   "lt"},
      {op_gt,   "gt"},
      {op_le,   "leq"},
      {op_ge,   "geq"},
      {op_eq2,  "eq2"},
      {op_neq2, "neq2"},
    };

    auto str_repr_it = tp_to_str.find(root->type());
    if (str_repr_it != tp_to_str.end()) {
      W << str_repr_it->second << " (" << lhs << ", " << rhs << ")";
      return;
    }
  }

  W << Operand{lhs, root->type(), true} <<
    " " << root_type_str << " " <<
    Operand{rhs, root->type(), false};
}

void compile_ternary_op(VertexAdaptor<op_ternary> root, CodeGenerator &W) {
  VertexPtr cond = root->cond();
  VertexPtr true_expr = root->true_expr();
  VertexPtr false_expr = root->false_expr();

  const TypeData *true_expr_tp, *false_expr_tp, *res_tp = nullptr;
  true_expr_tp = tinf::get_type(true_expr);
  false_expr_tp = tinf::get_type(false_expr);

  //TODO: optimize type_out
  if (type_out(true_expr_tp) != type_out(false_expr_tp)) {
    res_tp = tinf::get_type(root);
  }

  W << Operand{cond, root->type(), true} << " ? ";

  if (res_tp != nullptr) {
    W << TypeName(res_tp) << "(";
  }
  W << Operand{true_expr, root->type(), true};
  if (res_tp != nullptr) {
    W << ")";
  }

  W << " : ";

  if (res_tp != nullptr) {
    W << TypeName(res_tp) << "(";
  }
  W << Operand{false_expr, root->type(), true};
  if (res_tp != nullptr) {
    W << ")";
  }
}


void compile_if(VertexAdaptor<op_if> root, CodeGenerator &W) {
  W << "if (" << root->cond() << ") " <<
    BEGIN <<
    AsSeq{root->true_cmd()} <<
    END;

  if (root->has_false_cmd()) {
    W << " else " << root->false_cmd();
  }
}

void compile_while(VertexAdaptor<op_while> root, CodeGenerator &W) {
  W << "while (" << root->cond() << ") " <<
    CycleBody{root->cmd(), root->continue_label_id, root->break_label_id};
}


void compile_do(VertexAdaptor<op_do> root, CodeGenerator &W) {
  W << "do " <<
    BEGIN <<
    AsSeq{root->cmd()} <<
    Label{root->continue_label_id} <<
    END << " while (";
    W << root->cond();
  W << ");" << NL << Label{root->break_label_id};
}


void compile_return(VertexAdaptor<op_return> root, CodeGenerator &W) {
  bool resumable_flag = W.get_context().resumable_flag;
  if (resumable_flag) {
    if (root->has_expr()) {
      W << "RETURN " << MacroBegin{};
    } else {
      W << "RETURN_VOID " << MacroBegin{};
    }
  } else {
    W << "return ";
  }

  if (root->has_expr()) {
    W << root->expr();
  }

  if (resumable_flag) {
    W << MacroEnd{};
  }
}


void compile_for(VertexAdaptor<op_for> root, CodeGenerator &W) {
  W << "for (" <<
    JoinValues(*root->pre_cond(), ", ") << "; " <<
    JoinValues(*root->cond(), ", ") << "; " <<
    JoinValues(*root->post_cond(), ", ") << ") " <<
    CycleBody{root->cmd(), root->continue_label_id, root->break_label_id};
}

enum class func_call_mode {
  simple = 0, // just call
  async_call, // call of resumable function inside another resumable function, but not in fork
  fork_call // call using fork
};

void compile_func_call(VertexAdaptor<op_func_call> root, CodeGenerator &W, func_call_mode mode = func_call_mode::simple) {
  if (root->str_val == "make_clone" && tinf::get_type(root->args()[0])->is_primitive_type()) {
    // avoid generating make_clone call for primitive types such that (int, double, bool) just for beauty
    W << root->args()[0];
    return;
  }
  FunctionPtr func;
  if (root->extra_type == op_ex_internal_func) {
    W << root->str_val;
  } else {
    func = root->func_id;
    if (mode == func_call_mode::simple && W.get_context().resumable_flag && func->is_resumable) {
      static std::atomic<int> errors_count;
      int cnt = ++errors_count;
      if (cnt >= 10) {
        if (cnt == 10) {
          kphp_error(0, "Too many same errors about resumable functions, will skip others");
        }
      } else {
        kphp_error (0, fmt_format("Can't compile call of resumable function [{}] in too complex expression\n"
                                  "Consider using a temporary variable for this call.\n"
                                  "Function is resumable because of calls chain:\n"
                                  "{}",
                                  func->get_human_readable_name(), func->get_resumable_path()));
      }
    }


    if (mode == func_call_mode::fork_call) {
      W << FunctionForkName(func);
    } else {
      W << FunctionName(func);
    }
  }
  if (func && func->cpp_template_call) {
    const TypeData *tp = tinf::get_type(root);
    W << "< " << TypeName(tp) << " >";
  }
  W << "(";
  auto args = root->args();
  if (func && func->cpp_variadic_call) {
    if (args.size() == 1) {
      if (auto array = GenTree::get_actual_value(args[0]).try_as<op_array>()) {
        args = array->args();
      }
    }
  }

  W << JoinValues(vk::make_iterator_range(args.begin(), args.end()), ", ");
  W << ")";
}

void compile_func_call_fast(VertexAdaptor<op_func_call> root, CodeGenerator &W) {
  if (!root->func_id->can_throw) {
    compile_func_call(root, W);
    return;
  }
  bool is_void = root->rl_type == val_none;

  if (is_void) {
    W << "TRY_CALL_VOID_ " << MacroBegin{};
  } else {
    const TypeData *type = tinf::get_type(root);
    W << "TRY_CALL_ " << MacroBegin{} << TypeName(type) << ", ";
  }

  W.get_context().catch_labels.push_back("");
  compile_func_call(root, W);
  W.get_context().catch_labels.pop_back();

  W << ", ";
  compile_throw_action(W);
  W << MacroEnd{};
}

void compile_fork(VertexAdaptor<op_fork> root, CodeGenerator &W) {
  compile_func_call(root->func_call(), W, func_call_mode::fork_call);
}

void compile_async(VertexAdaptor<op_async> root, CodeGenerator &W) {
  auto func_call = root->func_call();
  if (root->has_save_var()) {
    W << root->save_var() << " = ";
  }
  compile_func_call(func_call, W, func_call_mode::async_call);
  FunctionPtr func = func_call->func_id;
  W << ";" << NL;
  W << (root->has_save_var() ? "TRY_WAIT" : "TRY_WAIT_DROP_RESULT");
  W << MacroBegin{} << gen_unique_name("resumable_label") << ", ";
  if (root->has_save_var()) {
    W << root->save_var() << ", ";
  }
  W << TypeName(tinf::get_type(func_call)) << MacroEnd{} << ";";
  if (func->can_throw) {
    W << NL;
    W << "CHECK_EXCEPTION" << MacroBegin{};
    compile_throw_action(W);
    W << MacroEnd{};
  }
}

void compile_foreach_ref_header(VertexAdaptor<op_foreach> root, CodeGenerator &W) {
  kphp_error(!W.get_context().resumable_flag, "foreach by reference is forbidden in resumable mode");
  auto params = root->params();

  //foreach (xs as [key =>] x)
  VertexPtr xs = params->xs();
  VertexPtr x = params->x();
  VertexPtr key;
  if (params->has_key()) {
    key = params->key();
  }

  string xs_copy_str;
  xs_copy_str = gen_unique_name("tmp_expr");
  const TypeData *xs_type = tinf::get_type(xs);

  W << BEGIN;
  //save array to 'xs_copy_str'
  W << TypeName(xs_type) << " &" << xs_copy_str << " = " << xs << ";" << NL;

  string it = gen_unique_name("it");
  W << "for (auto " << it << " = begin (" << xs_copy_str << "); " <<
    it << " != end (" << xs_copy_str << "); " <<
    "++" << it << ")" <<
    BEGIN;


  //save value
  W << TypeName(tinf::get_type(x)) << " &";
  W << x << " = " << it << ".get_value();" << NL;

  //save key
  if (key) {
    W << key << " = " << it << ".get_key();" << NL;
  }
}

void compile_foreach_noref_header(VertexAdaptor<op_foreach> root, CodeGenerator &W) {
  auto params = root->params();
  //foreach (xs as [key =>] x)
  VertexPtr x = params->x();
  VertexPtr xs = params->xs();
  VertexPtr key;
  auto temp_var = params->temp_var().as<op_var>();
  if (params->has_key()) {
    key = params->key();
  }

  TypeData const *type_data = xs->tinf_node.type_;
  if (auto xs_var = xs.try_as<op_var>()) {
    type_data = xs_var->var_id->tinf_node.type_;
  }

  PrimitiveType ptype = type_data->get_real_ptype();

  if (vk::none_of_equal(ptype, tp_array, tp_mixed)) {
    kphp_error_return(false, fmt_format("Invalid argument supplied for foreach() ({})", ptype_name(ptype)));
  }

  W << BEGIN;
  //save array to 'xs_copy_str'
  W << temp_var << " = " << xs << ";" << NL;
  W << temp_var << "$it = const_begin(" << temp_var << ");" << NL;
  W << temp_var << "$it$end = const_end(" << temp_var << ");" << NL;
  W << "for (; " << temp_var << "$it != " << temp_var << "$it$end; ++" << temp_var << "$it) " <<
    BEGIN;

  //save value
  W << x << " = " << temp_var << "$it" << ".get_value();" << NL;

  //save key
  if (key) {
    W << key << " = " << temp_var << "$it" << ".get_key();" << NL;
  }
}

void compile_foreach(VertexAdaptor<op_foreach> root, CodeGenerator &W) {
  auto params = root->params();
  auto cmd = root->cmd();

  //foreach (xs as [key =>] x)
  if (params->x()->ref_flag) {
    compile_foreach_ref_header(root, W);
  } else {
    compile_foreach_noref_header(root, W);
  }

  if (stage::has_error()) {
    return;
  }

  W << AsSeq{cmd} << NL <<
    Label{root->continue_label_id} <<
    END <<
    Label{root->break_label_id} << NL;
  if (!params->x()->ref_flag) {
    VertexPtr temp_var = params->temp_var();
    W << "clear_array(" << temp_var << ");" << NL;
  }
  W << END;
}

struct CaseInfo {
  size_t hash{0};
  bool is_default{false};
  string goto_name;
  CaseInfo *next{nullptr};
  VertexPtr v;
  VertexPtr expr;
  VertexPtr cmd;

  CaseInfo() = default;

  explicit CaseInfo(VertexPtr root) :
    is_default(root->type() == op_default),
    v(root) {
    if (auto cs = v.try_as<op_case>()) {
      expr = cs->expr();
      cmd = cs->cmd();

      VertexPtr val = GenTree::get_actual_value(expr);
      kphp_assert (val->type() == op_string);
      const string &s = val.as<op_string>()->str_val;
      hash = string_hash(s.c_str(), s.size());
    } else {
      cmd = v.as<op_default>()->cmd();
    }
  }
};

void compile_switch_str(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  auto cases_vertices = root->cases();
  vector<CaseInfo> cases(cases_vertices.size());
  std::transform(cases_vertices.begin(), cases_vertices.end(), cases.begin(), [](auto v) { return CaseInfo(v); });

  auto default_case_it = std::find_if(cases.begin(), cases.end(), vk::make_field_getter(&CaseInfo::is_default));
  auto default_case = default_case_it != cases.end() ? &(*default_case_it) : nullptr;

  int n = static_cast<int>(cases.size());
  std::map<unsigned int, int> hash_to_last_id;
  for (int i = n - 1; i >= 0; --i) {
    if (cases[i].is_default) {
      continue;
    }

    auto insert_res = hash_to_last_id.insert({cases[i].hash, i});
    bool inserted = insert_res.second;
    if (inserted) {
      cases[i].next = default_case;
    } else {
      auto prev_hash_and_id = insert_res.first;
      int next_i = prev_hash_and_id->second;
      prev_hash_and_id->second = i;
      cases[i].next = &cases[next_i];
    }

    auto next = cases[i].next;
    if (next && next->goto_name.empty()) {
      next->goto_name = gen_unique_name("switch_goto");
    }
  }

  auto temp_var_strval_of_condition = root->condition_on_switch();
  auto temp_var_matched_with_a_case = root->matched_with_one_case();

  W << BEGIN;
  W << temp_var_strval_of_condition << " = f$strval (" << root->condition() << ");" << NL;
  W << temp_var_matched_with_a_case << " = false;" << NL;

  W << "switch (" << temp_var_strval_of_condition << ".as_string().hash()) " << BEGIN;
  for (const auto &one_case : cases) {
    if (one_case.is_default) {
      W << "default:" << NL;
      if (!one_case.goto_name.empty()) {
        W << one_case.goto_name << ":;" << NL;
      }
      W << temp_var_matched_with_a_case << " = true;" << NL;
    } else {
      if (one_case.goto_name.empty()) {
        W << "case " << std::to_string(static_cast<long long>(one_case.hash)) << ":" << NL;
      } else {
        W << one_case.goto_name << ":;" << NL;
      }
      W << "if (!" << temp_var_matched_with_a_case << ") " << BEGIN;
      {
        W << "if (!equals (" << temp_var_strval_of_condition << ".as_string(), " << one_case.expr << ")) " << BEGIN;

        if (one_case.next) {
          W << "goto " << one_case.next->goto_name << ";" << NL;
        } else {
          W << "break;" << NL;
        }

        W << END << " else " << BEGIN;
        W << temp_var_matched_with_a_case << " = true;" << NL;
        W << END << NL;
      }
      W << END << NL;
    }
    W << one_case.cmd << NL;
  }
  W << END << NL;

  W << Label{root->continue_label_id} <<
    END <<
    Label{root->break_label_id};
}

void compile_switch_int(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  W << "switch (f$intval (" << root->condition() << "))" << BEGIN;

  W << "static_cast<void>(" << root->condition_on_switch() << ");" << NL;
  W << "static_cast<void>(" << root->matched_with_one_case() << ");" << NL;
  std::set<string> used;
  for (auto one_case : root->cases()) {
    VertexAdaptor<op_seq> cmd;
    if (auto cs = one_case.try_as<op_case>()) {
      cmd = cs->cmd();

      VertexPtr val = GenTree::get_actual_value(cs->expr());
      W << "case ";
      if (val->type() == op_int_const) {
        const string &str = val.as<op_int_const>()->str_val;
        W << str;
        kphp_error(used.insert(str).second, fmt_format("Switch: repeated cases found [{}]", str));
      } else {
        kphp_assert(is_const_int(val));
        W << val;
      }
    } else {
      W << "default";
      cmd = one_case.as<op_default>()->cmd();
    }
    W << ": " << cmd << NL;
  }

  W << Label{root->continue_label_id} << END;
  W << Label{root->break_label_id};
}


void compile_switch_var(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  string goto_name_if_default_in_the_middle;

  auto temp_var_condition_on_switch = root->condition_on_switch();
  auto temp_var_matched_with_a_case = root->matched_with_one_case();

  W << "do " << BEGIN;
  W << temp_var_condition_on_switch << " = " << root->condition() << ";" << NL;
  W << temp_var_matched_with_a_case << " = false;" << NL;

  for (const auto &one_case : root->cases()) {
    VertexAdaptor<op_seq> cmd;
    if (auto cs = one_case.try_as<op_case>()) {
      cmd = cs->cmd();
      W << "if (" << temp_var_matched_with_a_case << " || eq2(" << temp_var_condition_on_switch << ", " << cs->expr() << "))" << BEGIN;
      W << temp_var_matched_with_a_case << " = true;" << NL;
    } else {
      cmd = one_case.as<op_default>()->cmd();
      W << "if (" << temp_var_matched_with_a_case << ") " << BEGIN;
      goto_name_if_default_in_the_middle = gen_unique_name("switch_goto");
      W << goto_name_if_default_in_the_middle + ": ";
    }

    W << AsSeq{cmd};
    W << END << NL;
  }

  if (!goto_name_if_default_in_the_middle.empty()) {
    W << "if (" << temp_var_matched_with_a_case << ") " << BEGIN;
    W << "break;" << NL;
    W << END << NL;

    W << temp_var_matched_with_a_case << " = true;" << NL;
    W << "goto " << goto_name_if_default_in_the_middle << ";" << NL;
  }

  W << Label{root->continue_label_id} << END;
  W << " while(false);" << NL;
  W << Label{root->break_label_id};
}


void compile_switch(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  kphp_assert(root->condition_on_switch()->type() == op_var && root->matched_with_one_case()->type() == op_var);
  bool all_cases_are_ints = true;
  bool all_cases_are_strings = true;
  bool has_default = false;

  for (auto one_case : root->cases()) {
    if (one_case->type() == op_default) {
      kphp_error_return(!has_default, "Switch: several `default` cases found");
      has_default = true;
      continue;
    }

    auto val = GenTree::get_actual_value(one_case.as<op_case>()->expr());
    all_cases_are_ints    &= is_const_int(val);
    all_cases_are_strings &= (val->type() == op_string);
  }

  if (all_cases_are_strings) {
    compile_switch_str(root, W);
  } else if (all_cases_are_ints) {
    compile_switch_int(root, W);
  } else {
    compile_switch_var(root, W);
  }
}


bool compile_tracing_profiler(FunctionPtr func, CodeGenerator &W) {
  if (func->profiler_state == FunctionData::profiler_status::disable
      || (func->is_inline && func->profiler_state == FunctionData::profiler_status::enable_as_child)
      || !G->settings().profiler_level.get()) {
    return false;
  }

  const auto &location = func->root->get_location();
  const char *is_root = func->profiler_state == FunctionData::profiler_status::enable_as_root ? "true" : "false";
  vk::string_view file_name = location.file ? vk::string_view{location.file->file_name} : vk::string_view{"unknown"};
  if (file_name.starts_with(G->settings().base_dir.get())) {
    file_name.remove_prefix(G->settings().base_dir.get().size());
  }
  W << "struct TracingProfilerTraits " << BEGIN
    << "static constexpr const char *file_name() noexcept { return " << RawString(file_name) << "; }" << NL
    << "static constexpr const char *function_name() noexcept { return " << RawString(func->get_human_readable_name()) << "; }" << NL
    << "static constexpr size_t function_line() noexcept { return " << std::max(location.line, 0) << "; }" << NL
    << "static constexpr size_t profiler_level() noexcept { return " << G->settings().profiler_level.get() << "; }" << NL
    << "static constexpr bool is_root() noexcept { return " << is_root << "; }" << NL
    << END << ";" << NL;
  if (func->is_resumable) {
    W << "ResumableProfiler<TracingProfilerTraits> resumable_profiler;" << NL << NL;
    FunctionSignatureGenerator(W).set_final()
      << "bool run()" << BEGIN
      << "resumable_profiler.start(pos__);" << NL
      << "const bool done = run_profiling_resumable();" << NL
      << "resumable_profiler.stop(done);" << NL
      << "return done;" << NL << END << NL << NL;
  } else {
    W << "AutoProfiler<TracingProfilerTraits> auto_profiler;" << NL << NL;
  }

  return true;
}

void compile_function_resumable(VertexAdaptor<op_function> func_root, CodeGenerator &W) {
  FunctionPtr func = func_root->func_id;
  W << "//RESUMABLE FUNCTION IMPLEMENTATION" << NL;
  W << "class " << FunctionClassName(func) << " final : public Resumable " <<
    BEGIN <<
    "private:" << NL << Indent(+2);


  //MEMBER VARIABLES
  for (auto var : func->param_ids) {
    kphp_error(!var->is_reference, "reference function parametrs are forbidden in resumable mode");
    W << VarPlainDeclaration(var);
  }
  for (auto var : func->local_var_ids) {
    W << VarPlainDeclaration(var);         // inplace variables are stored as Resumable class fields as well
  }

  W << Indent(-2) << "public:" << NL << Indent(+2);

  //ReturnT
  W << "using ReturnT = " << TypeName(tinf::get_type(func, -1)) << ";" << NL;

  //CONSTRUCTOR
  FunctionSignatureGenerator(W) << FunctionClassName(func) << "(" << FunctionParams(func) << ")";
  if (!func->param_ids.empty()) {
    W << " :" << NL << Indent(+2);
    W << JoinValues(func->param_ids, ",", join_mode::multiple_lines,
                    [](CodeGenerator &W, VarPtr var) {
                      W << VarName(var) << "(" << VarName(var) << ")";
                    });
    if (!func->local_var_ids.empty()) {
      W << "," << NL;
    }
    W << JoinValues(func->local_var_ids, ",", join_mode::multiple_lines,
                    [](CodeGenerator &W, VarPtr var) {
                      W << VarName(var) << "()";
                    });
    W << Indent(-2);
  }
  W << " " << BEGIN << END << NL;

  if (compile_tracing_profiler(func, W)) {
    FunctionSignatureGenerator(W) << "bool run_profiling_resumable()" << BEGIN;
  } else {
    FunctionSignatureGenerator(W).set_final() << "bool run()" << BEGIN;
  }
  W << "RESUMABLE_BEGIN" << NL << Indent(+2);

  W << AsSeq{func_root->cmd()} << NL;

  W << Indent(-2) <<
    "RESUMABLE_END" << NL <<
    END << NL;


  W << Indent(-2);
  W << END << ";" << NL;

  //CALL FUNCTION
  W << FunctionDeclaration(func, false) << " " <<
    BEGIN;
  W << "return start_resumable < " << FunctionClassName(func) << "::ReturnT >" <<
    "(new " << FunctionClassName(func) << "(";

  const auto var_name_gen = [](CodeGenerator &W, VarPtr var) { W << VarName(var); };
  W << JoinValues(func->param_ids, ", ", join_mode::one_line, var_name_gen);

  W << "));" << NL;
  W << END << NL;

  //FORK FUNCTION
  W << FunctionForkDeclaration(func, false) << " " <<
    BEGIN;
  W << "return fork_resumable < " << FunctionClassName(func) << "::ReturnT >" <<
    "(new " << FunctionClassName(func) << "(";
  W << JoinValues(func->param_ids, ", ", join_mode::one_line, var_name_gen);
  W << "));" << NL;
  W << END << NL;

}

void compile_function(VertexAdaptor<op_function> func_root, CodeGenerator &W) {
  FunctionPtr func = func_root->func_id;

  W.get_context().parent_func = func;
  W.get_context().resumable_flag = func->is_resumable;

  if (func->is_resumable) {
    compile_function_resumable(func_root, W);
    return;
  }

  W << FunctionDeclaration(func, false) << " " << BEGIN;

  compile_tracing_profiler(func, W);

  for (auto var : func->local_var_ids) {
    if (var->type() != VarData::var_local_inplace_t && !var->is_foreach_reference) {
      W << VarDeclaration(var);
    }
  }

  if (func->has_variadic_param) {
    auto params = func->get_params();
    kphp_assert(!params.empty());
    auto variadic_arg = std::prev(params.end());
    auto name_of_variadic_param = VarName(variadic_arg->as<op_func_param>()->var()->var_id);
    W << "if (!" << name_of_variadic_param << ".is_vector())" << BEGIN;
    W << "php_warning(\"pass associative array(" << name_of_variadic_param << ") to variadic function: " << FunctionName(func) << "\");" << NL;
    W << name_of_variadic_param << " = f$array_values(" << name_of_variadic_param << ");" << NL;
    W << END << NL;
  }
  W << AsSeq{func_root->cmd()} << END << NL;
}

struct StrlenInfo {
  VertexPtr v;
  int len{0};
  bool str_flag{false};
  bool var_flag{false};
  string str;
};

static bool can_save_ref(VertexPtr v) {
  if (v->type() == op_var) {
    return true;
  }
  if (v->type() == op_func_call) {
    FunctionPtr func = v.as<op_func_call>()->func_id;
    if (func->is_extern()) {
      //todo
      return false;
    }
    return true;
  }
  return false;
}

void compile_string_build_as_string(VertexAdaptor<op_string_build> root, CodeGenerator &W) {
  vector<StrlenInfo> info(root->size());
  bool ok = true;
  bool was_dynamic = false;
  bool was_object = false;
  int static_length = 0;
  int ii = 0;
  for (auto i : root->args()) {
    info[ii].v = i;
    VertexPtr value = GenTree::get_actual_value(i);
    const TypeData *type = tinf::get_type(value);

    int value_length = type_strlen(type);
    if (value_length == STRLEN_ERROR) {
      kphp_error (0, fmt_format("Cannot convert type [{}] to string", type_out(type)));
      ok = false;
      ii++;
      continue;
    }

    if (value->type() == op_string) {
      info[ii].str_flag = true;
      info[ii].str = value->get_string();
      info[ii].len = (int)info[ii].str.size();
      static_length += info[ii].len;
    } else {
      if (value_length == STRLEN_DYNAMIC) {
        was_dynamic = true;
      } else if (value_length == STRLEN_OBJECT) {
        was_object = true;
      } else {
        if (value_length & STRLEN_WARNING_FLAG) {
          value_length &= ~STRLEN_WARNING_FLAG;
          kphp_warning (fmt_format("Suspicious conversion of type [{}] to string", type_out(type)));
        }

        kphp_assert (value_length >= 0);
        static_length += value_length;
      }

      info[ii].len = value_length;
    }

    ii++;
  }
  if (!ok) {
    return;
  }

  bool complex_flag = was_dynamic || was_object;
  std::string len_name;
  std::string tmp_string_name;
  if (complex_flag) {
    W << "(" << BEGIN;
    vector<string> to_add;
    for (auto &str_info : info) {
      if (str_info.str_flag) {
        continue;
      }
      if (str_info.len == STRLEN_DYNAMIC || str_info.len == STRLEN_OBJECT) {
        string var_name = gen_unique_name("var");

        if (str_info.len == STRLEN_DYNAMIC) {
          bool can_save_ref_flag = can_save_ref(str_info.v);
          W << "const " << TypeName(tinf::get_type(str_info.v)) << " " <<
            (can_save_ref_flag ? "&" : "") <<
            var_name << "=" << str_info.v << ";" << NL;
        } else if (str_info.len == STRLEN_OBJECT) {
          W << "const string " << var_name << " = f$strval" <<
            "(" << str_info.v << ");" << NL;
        }

        to_add.push_back(var_name);
        str_info.var_flag = true;
        str_info.str = var_name;
      }
    }

    len_name = gen_unique_name("strlen");
    W << "string::size_type " << len_name << " = " << static_length;
    for (const auto &str : to_add) {
      W << " + max_string_size (" << str << ")";
    }
    W << ";" << NL;
    tmp_string_name = gen_unique_name("tmp_string");
    W << "string " << tmp_string_name << " = ";
  }

  W << "string (";
  if (complex_flag) {
    W << len_name;
  } else {
    W << static_length;
  }
  W << ", true)";
  for (const auto &str_info : info) {
    W << ".append_unsafe (";
    if (str_info.str_flag) {
      W << RawString(str_info.str) << ", " << str_info.len;
    } else if (str_info.var_flag) {
      W << str_info.str;
    } else {
      W << str_info.v;
    }
    W << ")";
  }
  W << ".finish_append()";

  if (complex_flag) {
    W << ";" << NL
      << tmp_string_name << ";" << NL
      << END << ")";
  }
}

void compile_index_of_array(VertexAdaptor<op_index> root, CodeGenerator &W) {
  bool used_as_rval = root->rl_type != val_l;
  if (!used_as_rval) {
    kphp_assert(root->has_key());
    W << root->array() << "[" << root->key() << "]";
  } else {
    W << root->array() << ".get_value (" << root->key();
    // if it's a const string key access like $a['somekey'],
    // compute the 'somekey' string hash during the compile time and call array<T>::get_value(string, precomputed_hash)
    if (auto precomputed_hash = can_use_precomputed_hash_indexing_array(root->key())) {
      W << ", " << precomputed_hash << "L";
    }
    W << ")";
  }
}

void compile_index_of_string(VertexAdaptor<op_index> root, CodeGenerator &W) {
  kphp_assert(root->has_key());

  W << root->array() << ".get_value(" << root->key() << ")";
}

void compile_instance_prop(VertexAdaptor<op_instance_prop> root, CodeGenerator &W) {
  W << root->instance() << "->$" << root->get_string();
}

void compile_index(VertexAdaptor<op_index> root, CodeGenerator &W) {
  PrimitiveType array_ptype = root->array()->tinf_node.get_type()->ptype();

  switch (array_ptype) {
    case tp_string:
      compile_index_of_string(root, W);
      break;
    case tp_tuple:
      W << TupleGetIndex(root->array(), root->key());
      break;
    case tp_shape:
      W << ShapeGetIndex(root->array(), root->key());
      break;
    default:
      compile_index_of_array(root, W);
  }
}


void compile_seq_rval(VertexPtr root, CodeGenerator &W) {
  kphp_assert(root->size());

  W << "(" << BEGIN;        // gcc "statement exprs" feature: ({ ...; v$result_var; })
  for (auto i : *root) {
    W << i << ";" << NL;    // last statement evaluation result will be used as a whole block result
  }
  W << END << ")";
}

void compile_xset(VertexAdaptor<meta_op_xset> root, CodeGenerator &W) {
  auto arg = root->expr();
  // separately: isset($arr[idx]) — is v$arr.isset(idx) (same for the unset)
  // but if $arr is a tuple/shape, then it's not an .isset method but is_null on the actual element
  if (auto index = arg.try_as<op_index>()) {
    kphp_assert (index->has_key());
    if (vk::any_of_equal(tinf::get_type(index->array())->ptype(), tp_array, tp_mixed)) {
      W << "(" << index->array();
      if (root->type() == op_isset) {
        W << ").isset (";
      } else if (root->type() == op_unset) {
        W << ").unset (";
      } else {
        kphp_assert (0);
      }
      W << index->key() << ")";
      return;
    }
  }
  if (root->type() == op_unset) {
    W << "unset (" << arg << ")";
  } else {
    W << "(!f$is_null(" << arg << "))";
  }
}

void compile_list(VertexAdaptor<op_list> root, CodeGenerator &W) {
  VertexPtr arr = root->array();
  VertexRange list = root->list();
  PrimitiveType ptype = tinf::get_type(arr)->get_real_ptype();
  kphp_assert(vk::any_of_equal(ptype, tp_array, tp_mixed, tp_tuple, tp_shape));

  for (const auto x : list) {
    const auto kv = x.as<op_list_keyval>();

    if (ptype == tp_tuple) {
      const auto index = kv->key().as<op_int_const>();
      W << "assign (" << kv->var() << ", " << TupleGetIndex(arr, index->str_val) << ");" << NL;
    } else if (ptype == tp_shape) {
      W << "assign (" << kv->var() << ", " << ShapeGetIndex(arr, kv->key()) << ");" << NL;
    } else {
      W << "assign (" << kv->var() << ", " << arr << ".get_value (" << kv->key() << "));" << NL;
    }
  }
}


void compile_array(VertexAdaptor<op_array> root, CodeGenerator &W) {
  int n = (int)root->args().size();
  const TypeData *type = tinf::get_type(root);

  if (n == 0) {
    W << TypeName(type) << " ()";
    return;
  }

  bool has_double_arrow = false;
  int int_cnt = 0, string_cnt = 0, xx_cnt = 0;
  for (size_t key_id = 0; key_id < root->args().size(); ++key_id) {
    if (auto arrow = root->args()[key_id].try_as<op_double_arrow>()) {
      VertexPtr key = GenTree::get_actual_value(arrow->key());
      if (!has_double_arrow && key->type() == op_int_const) {
        if (key.as<op_int_const>()->str_val == std::to_string(key_id)) {
          root->args()[key_id] = arrow->value();
          continue;
        }
      }
      has_double_arrow = true;
      PrimitiveType tp = tinf::get_type(key)->ptype();
      if (tp == tp_int) {
        int_cnt++;
      } else {
        if (tp == tp_string && key->type() == op_string) {
          const string &key_str = key.as<op_string>()->str_val;
          if (php_is_int(key_str.c_str(), key_str.size())) {
            int_cnt++;
          } else {
            string_cnt++;
          }
        } else {
          xx_cnt++;
        }
      }
    } else {
      int_cnt++;
    }
  }
  if (n <= 10 && !has_double_arrow && type->ptype() == tp_array && root->extra_type != op_ex_safe_version) {
    W << TypeName(type) << "::create(" << JoinValues(*root, ", ") << ")";
    return;
  }

  W << "(" << BEGIN;

  string arr_name = "tmp_array";
  W << TypeName(type) << " " << arr_name << " = ";

  //TODO: check
  if (type->ptype() == tp_array) {
    W << TypeName(type);
  } else {
    W << "array <mixed>";
  }
  W << " (array_size ("
    << int_cnt + xx_cnt << ", "
    << string_cnt + xx_cnt << ", "
    << (has_double_arrow ? "false" : "true") << "));" << NL;
  for (auto cur : root->args()) {
    W << arr_name;
    if (auto arrow = cur.try_as<op_double_arrow>()) {
      W << ".set_value (" << arrow->key() << ", " << arrow->value();
      if (auto precomputed_hash = can_use_precomputed_hash_indexing_array(arrow->key())) {
        W << ", " << precomputed_hash << "L";
      }
      W << ")";
    } else {
      W << ".push_back (" << cur << ")";
    }

    W << ";" << NL;
  }

  W << arr_name << ";" << NL <<
    END << ")";
}

void compile_tuple(VertexAdaptor<op_tuple> root, CodeGenerator &W) {
  W << "std::make_tuple(" << JoinValues(root->args(), ", ") << ")";
}

void compile_shape(VertexAdaptor<op_shape> root, CodeGenerator &W) {
  // important! values are sorted in the keys hash ascending order;
  // this is in sync with how shape cpp-layout is generated in type_out_impl()
  std::vector<std::pair<int64_t, VertexPtr>> sorted_by_hash;
  sorted_by_hash.reserve(root->args().size());
  for (auto double_arrow : *root) {
    const std::string &key_str = GenTree::get_actual_value(double_arrow.as<op_double_arrow>()->lhs())->get_string();
    sorted_by_hash.emplace_back(string_hash(key_str.c_str(), key_str.size()), double_arrow.as<op_double_arrow>()->rhs());
  }
  std::sort(sorted_by_hash.begin(), sorted_by_hash.end(), [](const auto &a, const auto &b) -> bool {
    kphp_assert(a.first != b.first);
    return a.first < b.first;
  });

  const auto val_gen = [](CodeGenerator &W, const std::pair<int64_t, VertexPtr> &hash_and_rhs) {
    W << hash_and_rhs.second;
  };

  const char *sep = W.get_context().inside_macro ? " COMMA " : ", ";
  W << TypeName(tinf::get_type(root)) << "{shape_variadic_constructor_stub{} " << sep <<
    JoinValues(sorted_by_hash, sep, join_mode::one_line, val_gen) << "}";
}

void compile_func_ptr(VertexAdaptor<op_func_ptr> root, CodeGenerator &W) {
  /**
   * KPHP code like this:
   *   array_map(function ($x) { return $x; }, ['a', 'b']);
   *
   * Will be transformed to:
   *   array_map([bound_class = anon$$__construct()] (string x) {
   *       return anon$$__invoke(bound_class, std::move(x));
   *   }), const_array);
   */
  vk::string_view name_bound_class;
  if (root->func_id->is_lambda()) {
    name_bound_class = "bound_class";
    W << "[" << name_bound_class << " = " << root->bound_class() << "]";
  } else {
    W << "[]";
  }

  FunctionSignatureGenerator(W) << "(auto &&... args) " << BEGIN;
  {
    W << "return " << FunctionName(root->func_id) << "(";
    if (!name_bound_class.empty()) {
      W << name_bound_class << ",";
    }
    W << "std::forward<decltype(args)>(args)...);";
  }
  W << END;
}

void compile_defined(VertexPtr root __attribute__((unused)), CodeGenerator &W __attribute__((unused))) {
  W << "false";
  //TODO: it is not CodeGen part
}

void compile_safe_version(VertexPtr root, CodeGenerator &W) {
  if (auto set_value = root.try_as<op_set_value>()) {
    W << "SAFE_SET_VALUE " << MacroBegin{} <<
      set_value->array() << ", " <<
      set_value->key() << ", " <<
      TypeName(tinf::get_type(set_value->key())) << ", " <<
      set_value->value() << ", " <<
      TypeName(tinf::get_type(set_value->value())) <<
      MacroEnd{};
  } else if (OpInfo::rl(root->type()) == rl_set) {
    auto op = root.as<meta_op_binary>();
    if (OpInfo::type(root->type()) == binary_func_op) {
      W << "SAFE_SET_FUNC_OP " << MacroBegin{};
    } else if (OpInfo::type(root->type()) == binary_op) {
      W << "SAFE_SET_OP " << MacroBegin{};
    } else {
      kphp_fail();
    }
    W << op->lhs() << ", " <<
      OpInfo::str(root->type()) << ", " <<
      op->rhs() << ", " <<
      TypeName(tinf::get_type(op->rhs())) <<
      MacroEnd{};
  } else if (auto pb = root.try_as<op_push_back>()) {
    W << "SAFE_PUSH_BACK " << MacroBegin{} <<
      pb->array() << ", " <<
      pb->value() << ", " <<
      TypeName(tinf::get_type(pb->value())) <<
      MacroEnd{};
  } else if (auto pb = root.try_as<op_push_back_return>()) {
    W << "SAFE_PUSH_BACK_RETURN " << MacroBegin{} <<
      pb->array() << ", " <<
      pb->value() << ", " <<
      TypeName(tinf::get_type(pb->value())) <<
      MacroEnd{};
  } else if (root->type() == op_array) {
    compile_array(root.as<op_array>(), W);
    return;
  } else if (auto index = root.try_as<op_index>()) {
    kphp_assert (index->has_key());
    W << "SAFE_INDEX " << MacroBegin{} <<
      index->array() << ", " <<
      index->key() << ", " <<
      TypeName(tinf::get_type(index->key())) <<
      MacroEnd{};
  } else {
    kphp_error (0, fmt_format("Safe version of [{}] is not supported", OpInfo::str(root->type())));
    kphp_fail();
  }

}


void compile_set_value(VertexAdaptor<op_set_value> root, CodeGenerator &W) {
  W << "(" << root->array() << ").set_value (" << root->key() << ", " << root->value();
  if (auto precomputed_hash = can_use_precomputed_hash_indexing_array(root->key())) {
    W << ", " << precomputed_hash << "L";
  }
  W << ")";
  // there is no dedicated string/tuple overloadings for the set_value() (unlike in compile_index()),
  // as when these are used in lvalue context string will be generalized to a var
  // and a tuple will give an error; in the end we get only array/var here
}

void compile_push_back(VertexAdaptor<op_push_back> root, CodeGenerator &W) {
  W << "(" << root->array() << ").push_back (" << root->value() << ")";
}

void compile_push_back_return(VertexAdaptor<op_push_back_return> root, CodeGenerator &W) {
  W << "(" << root->array() << ").push_back_return (" << root->value() << ")";
}


void compile_string(VertexAdaptor<op_string> root, CodeGenerator &W) {
  W << "string (" << RawString(root->str_val) << ", " << root->str_val.size() << ")";
}

void compile_string_build(VertexAdaptor<op_string_build> root, CodeGenerator &W) {
  compile_string_build_as_string(root, W);
}

void compile_break_continue(VertexAdaptor<meta_op_goto> root, CodeGenerator &W) {
  if (root->int_val != 0) {
    W << "goto " << LabelName{root->int_val};
  } else {
    if (root->type() == op_break) {
      W << "break";
    } else if (root->type() == op_continue) {
      W << "continue";
    } else {
      assert (0);
    }
  }
}

template<Operation op_conv_l>
void compile_conv_l(VertexAdaptor<op_conv_l> root, CodeGenerator &W) {
  static_assert(vk::any_of_equal(op_conv_l, op_conv_array_l, op_conv_int_l, op_conv_string_l), "unexpected conv_op");
  VertexPtr val = root->expr();
  PrimitiveType tp = tinf::get_type(val)->get_real_ptype();
  if (tp == tp_mixed ||
      (op_conv_l == op_conv_array_l && tp == tp_array) ||
      (op_conv_l == op_conv_int_l && tp == tp_int) ||
      (op_conv_l == op_conv_string_l && tp == tp_string)) {
    std::string fun_name = "unknown";
    if (auto cur_fun = stage::get_function()) {
      fun_name = cur_fun->get_human_readable_name();
    }
    W << OpInfo::str(op_conv_l) << " (" << val << ", R\"(" << fun_name << ")\")";
  } else {
    kphp_error (0, fmt_format("Trying to pass incompatible type as reference to '{}'", root->get_string()));
  }
}

void compile_cycle_op(VertexPtr root, CodeGenerator &W) {
  Operation tp = root->type();
  switch (tp) {
    case op_while:
      compile_while(root.as<op_while>(), W);
      break;
    case op_do:
      compile_do(root.as<op_do>(), W);
      break;
    case op_for:
      compile_for(root.as<op_for>(), W);
      break;
    case op_foreach:
      compile_foreach(root.as<op_foreach>(), W);
      break;
    case op_switch:
      compile_switch(root.as<op_switch>(), W);
      break;
    default:
      assert (0);
      break;
  }
}


void compile_common_op(VertexPtr root, CodeGenerator &W) {
  Operation tp = root->type();
  string str;
  switch (tp) {
    case op_seq:
      W << BEGIN << AsSeq{root.as<op_seq>()} << END;
      break;
    case op_seq_rval:
      compile_seq_rval(root, W);
      break;

    case op_int_const:
      str = root.as<op_int_const>()->str_val;
      if (str.size() > 9) {
        W << "(int64_t)";
      }
      W << str << "L";
      break;
    case op_float_const:
      str = root.as<op_float_const>()->str_val;
      W << "(double)" << str;
      break;
    case op_false:
      W << "false";
      break;
    case op_true:
      W << "true";
      break;
    case op_null:
      W << "Optional<bool>{}";
      break;
    case op_var:
      W << VarName(root.as<op_var>()->var_id);
      break;
    case op_string:
      compile_string(root.as<op_string>(), W);
      break;

    case op_if:
      compile_if(root.as<op_if>(), W);
      break;
    case op_return:
      compile_return(root.as<op_return>(), W);
      break;
    case op_global:
    case op_static:
      //already processed
      break;
    case op_throw:
      compile_throw(root.as<op_throw>(), W);
      break;
    case op_continue:
    case op_break:
      compile_break_continue(root.as<meta_op_goto>(), W);
      break;
    case op_try:
      compile_try(root.as<op_try>(), W);
      break;
    case op_fork:
      compile_fork(root.as<op_fork>(), W);
      break;
    case op_async:
      compile_async(root.as<op_async>(), W);
      break;
    case op_function:
      compile_function(root.as<op_function>(), W);
      break;
    case op_func_call:
      compile_func_call_fast(root.as<op_func_call>(), W);
      break;
    case op_func_ptr:
      compile_func_ptr(root.as<op_func_ptr>(), W);
      break;
    case op_string_build:
      compile_string_build(root.as<op_string_build>(), W);
      break;
    case op_index:
      compile_index(root.as<op_index>(), W);
      break;
    case op_instance_prop:
      compile_instance_prop(root.as<op_instance_prop>(), W);
      break;
    case op_isset:
      compile_xset(root.as<meta_op_xset>(), W);
      break;
    case op_list:
      compile_list(root.as<op_list>(), W);
      break;
    case op_array:
      compile_array(root.as<op_array>(), W);
      break;
    case op_tuple:
      compile_tuple(root.as<op_tuple>(), W);
      break;
    case op_shape:
      compile_shape(root.as<op_shape>(), W);
      break;
    case op_unset:
      compile_xset(root.as<meta_op_xset>(), W);
      break;
    case op_empty:
    case op_phpdoc_var:
      break;
    case op_defined:
      compile_defined(root.as<op_defined>(), W);
      break;
    case op_conv_array_l:
      compile_conv_l(root.as<op_conv_array_l>(), W);
      break;
    case op_conv_int_l:
      compile_conv_l(root.as<op_conv_int_l>(), W);
      break;
    case op_conv_string_l:
      compile_conv_l(root.as<op_conv_string_l>(), W);
      break;
    case op_set_value:
      compile_set_value(root.as<op_set_value>(), W);
      break;
    case op_push_back:
      compile_push_back(root.as<op_push_back>(), W);
      break;
    case op_push_back_return:
      compile_push_back_return(root.as<op_push_back_return>(), W);
      break;
    case op_noerr:
      compile_noerr(root.as<op_noerr>(), W);
      break;
    case op_clone: {
      auto tp = tinf::get_type(root);
      if (auto klass = tp->class_type()) {
        if (klass->is_class()) {
          W << root.as<op_clone>()->expr() << ".clone()";
          break;
        }
      }
      kphp_error_return(false, "unsupported operand for cloning");
      break;
    }
    case op_alloc: {
      const TypeData *tp = tinf::get_type(root);
      kphp_assert(tp->ptype() == tp_Class);
      auto alloc_function = tp->class_type()->is_empty_class() ? "().empty_alloc()" : "().alloc()";
      W << TypeName(tp) << alloc_function;
      break;
    }
    default:
      kphp_fail();
      break;
  }
}


void compile_vertex(VertexPtr root, CodeGenerator &W) {
  OperationType tp = OpInfo::type(root->type());

  W << UpdateLocation(root->location);

  bool close_par = root->val_ref_flag == val_r || root->val_ref_flag == val_l;

  if (root->val_ref_flag == val_r) {
    W << "val(";
  } else if (root->val_ref_flag == val_l) {
    W << "ref(";
  }

  if (root->extra_type == op_ex_safe_version) {
    compile_safe_version(root, W);
  } else {
    switch (tp) {
      case prefix_op:
        compile_prefix_op(root.as<meta_op_unary>(), W);
        break;
      case postfix_op:
        compile_postfix_op(root.as<meta_op_unary>(), W);
        break;
      case binary_op:
      case binary_func_op:
        compile_binary_op(root.as<meta_op_binary>(), W);
        break;
      case ternary_op:
        compile_ternary_op(root.as<op_ternary>(), W);
        break;
      case common_op:
        compile_common_op(root, W);
        break;
      case cycle_op:
        compile_cycle_op(root, W);
        break;
      case conv_op:
        compile_conv_op(root.as<meta_op_unary>(), W);
        break;
      default:
        fmt_print("{}: {}\n", tp, root->type());
        assert (0);
        break;
    }
  }

  if (close_par) {
    W << ")";
  }
}
