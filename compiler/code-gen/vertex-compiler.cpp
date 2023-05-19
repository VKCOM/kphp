// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/vertex-compiler.h"

#include <iterator>
#include <unordered_map>

#include "common/wrappers/field_getter.h"

#include "common/wrappers/likely.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/files/json-encoder-tags.h"
#include "compiler/code-gen/files/tracing-autogen.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/ffi-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/type-hint.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"
#include "compiler/vertex.h"
#include "compiler/vertex-util.h"

namespace {

struct IntLit {
  vk::string_view value;

  void compile(CodeGenerator &W) const {
    if (unlikely(value == "-9223372036854775808")) {
      // avoid "integer overflow" warning for PHP_INT_MIN values
      W << "(-9223372036854775807_i64 - 1_i64)";
    } else {
      W << value << "_i64";
    }
  }
};

// TmpExpr wraps a part of expression that is being extracted to a separate statement
// to serve as an intermediate value. It's necessary to use this wrapper to avoid
// any issues related to the expression lifetimes expectations.
struct TmpExpr {
  VertexPtr root;

  TmpExpr(VertexPtr root) : root{root} {}

  const TypeData *get_type() {
    const auto *type = tinf::get_type(root);
    if (type->ptype() == tp_tmp_string) {
      // tmp_string must be materialized if it's used as an intermediate value
      // tmp_string is never optional, so we don't have to worry about the flags
      return TypeData::get_type(tp_string);
    }
    return type;
  }

  void compile(CodeGenerator &W) const {
    const auto *type = tinf::get_type(root);
    if (type->ptype() == tp_tmp_string) {
      W << "materialize_tmp_string(" << root << ")";
      return;
    }
    W << root;
  }
};

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
    int_index(VertexUtil::get_actual_value(key)->get_string()) {
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
    index(VertexUtil::get_actual_value(key)->get_string()) {
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

struct EmptyReturn {
  //TODO: it's copypasted to compile_return
  void compile(CodeGenerator &W) const {
    CGContext &context = W.get_context();
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
  }
};

struct ThrowAction {
  //TODO: some interface for context?
  static void compile(CodeGenerator &W) {
    CGContext &context = W.get_context();
    if (context.catch_labels.empty() || context.catch_labels.back().empty()) {
      W << EmptyReturn();
    } else {
      W << "goto " << context.catch_labels.back();
      context.catch_label_used.back() = 1;
    }
  }
};

void compile_prefix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  W << OpInfo::str(root->type()) << Operand{root->expr(), root->type(), true};
}

void compile_postfix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  W << Operand{root->expr(), root->type(), true} << OpInfo::str(root->type());
}

void compile_ffi_c2php(VertexPtr expr, bool is_ref, const TypeHint *php_type, CodeGenerator &W) {
  if (const FFIType *ffi_type = FFIRoot::get_ffi_type(php_type)) {
    if (ffi_type->kind == FFITypeKind::Array && ffi_type->num != -1) {
      W << "ffi_c2php_array(" << expr << ", " << ffi_type->num << ")";
      return;
    }
  }
  const char *func = is_ref ? "ffi_c2php_ref" : "ffi_c2php";
  W << func << "(" << expr << ")";
}

void compile_ffi_c2php_conv(VertexAdaptor<op_ffi_c2php_conv> root, CodeGenerator &W) {
  compile_ffi_c2php(root->expr(), tinf::get_type(root)->is_ffi_ref(), root->php_type, W);
}

void compile_ffi_php2c_tag(const FFIType *c_type, const std::string scope_name, CodeGenerator &W) {
  const FFIType *type = c_type->kind == FFITypeKind::Var ? c_type->members[0] : c_type;
  std::string tag;
  // some tags have unboxed specializations, other types are specialized with CData templates;
  // see ffi_tag<> specializations in ffi.h for more info
  if (type->is_void_ptr()) {
    tag = type->members[0]->is_const() ? "const void*" : "void*";
  } else if (type->is_cstring()) {
    tag = "const char*";
  } else if (vk::any_of_equal(type->kind, FFITypeKind::Pointer, FFITypeKind::Struct, FFITypeKind::StructDef, FFITypeKind::Union, FFITypeKind::UnionDef)) {
    tag = "C$FFI$CData<" + ffi_mangled_decltype_string(scope_name, type) + ">";
  } else {
    tag = ffi_mangled_decltype_string(scope_name, type);
  }
  W << "ffi_tag<" << tag << ">{}";
}

void compile_ffi_php2c(VertexPtr expr, const TypeHint *c_type, CodeGenerator &W) {
  W << "ffi_php2c(" << expr << ", ";
  compile_ffi_php2c_tag(FFIRoot::get_ffi_type(c_type), FFIRoot::get_ffi_scope(c_type), W);
  W << ")";
}

void compile_ffi_php2c_conv(VertexAdaptor<op_ffi_php2c_conv> root, CodeGenerator &W) {
  // just to make the generated code a bit more compact
  if (root->expr()->type() == op_null) {
    W << "nullptr";
    return;
  }
  compile_ffi_php2c(root->expr(), root->c_type, W);
}

void compile_conv_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  if (root->type() == op_conv_regexp) {
    W << root->expr();
  } else if (root->type() == op_ffi_php2c_conv) {
    compile_ffi_php2c_conv(root.as<op_ffi_php2c_conv>(), W);
  } else if (root->type() == op_ffi_c2php_conv) {
    compile_ffi_c2php_conv(root.as<op_ffi_c2php_conv>(), W);
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

void compile_throw(VertexAdaptor<op_throw> root, CodeGenerator &W) {
  W << BEGIN <<
    "THROW_EXCEPTION " << MacroBegin{} << root->exception() << MacroEnd{} << ";" << NL <<
    ThrowAction() << ";" << NL <<
    END << NL;
}

void compile_try(VertexAdaptor<op_try> root, CodeGenerator &W) {
  auto move_exception = [&](ClassPtr caught_class, VertexAdaptor<op_var> dst) {
    if (caught_class->name == "Throwable") {
      W << dst << " = std::move(CurException);" << NL;
      return;
    }
    std::string e = gen_unique_name("e");
    W << BEGIN << "auto " << e << " = std::move(CurException);" << NL <<
         dst << " = " << e << ".template cast_to<" << caught_class->src_name << ">();" << NL << END << NL;
    // we don't allow catching arbitrary classes, but we don't check
    // interfaces at compile time; to be on the safe side, check that
    // exception dynamic_cast succeeded
    if (caught_class->is_interface()) {
      W << "php_assert(!" << dst << ".is_null());" << NL;
    }
  };

  CGContext &context = W.get_context();

  std::string catch_label = gen_unique_name("catch_label");
  W << "/""*** TRY ***""/" << NL;
  context.catch_labels.push_back(catch_label);
  context.catch_label_used.push_back(0);
  W << root->try_cmd() << NL;
  context.catch_labels.pop_back();
  bool used = context.catch_label_used.back();
  context.catch_label_used.pop_back();

  if (used) {
    // A catch list produces a series of if/elseif statements ending with else.
    //
    // Every if condition tries to match an exception against the type being
    // handled by the catch clause.
    // The "else" contains the ThrowAction (return) that makes exception
    // escape the current function.
    //
    // If try block is marked with catches_all=true, then instead of the
    // normal else branch with ThrowAction we'll generate an unconditional exception move.

    W << "/""*** CATCH ***""/" << NL;
    W << "if (0) " << BEGIN <<
      catch_label << ":;" << NL; // TODO: Label (label_id) ?

    bool need_else = !root->catches_all;
    bool is_first_catch = true;
    auto last_catch = root->catch_list().back();
    for (auto &c : root->catch_list()) {
      auto catch_op = c.as<op_catch>();
      bool is_last_catch = c == last_catch;

      ClassPtr caught_class = catch_op->exception_class;

      W << UpdateLocation(catch_op->var()->location);

      if (is_last_catch && root->catches_all) {
        // the last catch in a try statement that "catches all" is guaranteed not to fail;
        //
        // imagine this case:
        //     try {
        //       // something that throws \Exception
        //     } catch (\Exception $e) {}
        // the \Exception handling catch is both last and first catch and try.catches_all=true,
        // so we can generate the move assignment without any checks
        if (!is_first_catch) {
          W << "else " << BEGIN << NL;
        }
        move_exception(caught_class, catch_op->var());
        W << catch_op->cmd() << NL;
        if (!is_first_catch) {
          W << END << NL;
        }
      } else {
        W << (is_first_catch ? "if" : "else if");
        if (caught_class->derived_classes.empty()) {
          W << " (f$get_hash_of_class(CurException) == " << caught_class->get_hash() << ") ";
        } else {
          W << " (f$is_a<" << caught_class->src_name << ">(CurException)) ";
        }
        std::string e = gen_unique_name("e");
        W << BEGIN;
        move_exception(caught_class, catch_op->var());
        W << catch_op->cmd() << NL << END << NL;
      }

      is_first_catch = false;
    }

    if (need_else) {
      W << "else " << BEGIN <<
        ThrowAction() << ";" << NL <<
        END << NL;
    }

    W << END << NL;
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
  W << "NullCoalesce< " << TypeName{type} << " >(";
  const auto index = lhs.try_as<op_index>();
  const auto array_ptype = index ? tinf::get_type(index->array())->get_real_ptype() : tp_any;
  if (index && vk::none_of_equal(array_ptype, tp_shape, tp_tuple)) {
    kphp_assert (index->has_key());
    W << index->array() << ", " << index->key();
    if (vk::any_of_equal(array_ptype, tp_array, tp_mixed)) {
      if (auto precomputed_hash = can_use_precomputed_hash_indexing_array(index->key())) {
        W << ", " << precomputed_hash << "_i64";
      }
    }
  } else {
    W << lhs;
  }

  W << ").finalize(";

  if (vk::any_of_equal(rhs->type(), op_var, op_int_const, op_float_const, op_false, op_null)) {
    W << rhs;
  } else {
    auto &context = W.get_context();
    context.catch_labels.emplace_back();
    ++context.inside_null_coalesce_fallback;
    FunctionSignatureGenerator(W) << "[&] ()";
    W << " -> " << TypeName{tinf::get_type(rhs)} << " " << BEGIN
      << " return " << rhs << ";" << NL
      << END;
    context.catch_labels.pop_back();
    kphp_assert(context.inside_null_coalesce_fallback > 0);
    context.inside_null_coalesce_fallback--;
  }

  W << ")";
  if (rhs->throw_flag) {
    W << ", " << ThrowAction{} << MacroEnd{};
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

bool try_compile_append_inplace(VertexAdaptor<op_set_dot> root, CodeGenerator &W);

void compile_binary_op(VertexAdaptor<meta_op_binary> root, CodeGenerator &W) {
  const auto &root_type_str = OpInfo::str(root->type());
  kphp_error_return (root_type_str[0] != '@', fmt_format("Unexpected {}\n", vk::string_view{root_type_str}.substr(1)));

  auto lhs = root->lhs();
  auto rhs = root->rhs();

  const auto *lhs_tp = tinf::get_type(lhs);
  const auto *rhs_tp = tinf::get_type(rhs);

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
      const TypeData *res_tp = tinf::get_type(root)->lookup_at_any_key();
      W << "array_add < " << TypeName(res_tp) << " > (" << lhs << ", " << rhs << ")";
      return;
    }
  }
  // special inplace variables that are defined at the assignment location, not in the beginning of the function
  if (root->type() == op_set && lhs->type() == op_var && lhs->extra_type == op_ex_var_superlocal_inplace) {
    W << TypeName(lhs_tp) << " ";    // generates "array<T> $tmp = v" instead of "$tmp = v"
  }

  if (root->type() == op_set_dot && root->rl_type == val_none) {
    if (try_compile_append_inplace(root.as<op_set_dot>(), W)) {
      return;
    }
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
      {op_le,   "leq"},
      {op_eq2,  "eq2"},
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

void compile_ffi_callback(VertexAdaptor<op_callback_of_builtin> callback, const FFIType *fn_type, const std::string &scope_name, CodeGenerator &W) {
  // for FFI callback the code generation is close to what we're doing for normal op_callback_of_builtin,
  // except we need to add some prologue and epilogue to the function being passed;
  // the prologue converts all callback arguments with c2php (C calls callback with C types, KPHP expects PHP types)
  // the epilogue converts the call result with php2c (KPHP returns PHP types, C side expects C types)

  W << LockComments{};

  // the outermost lambda params have C types, not PHP types,
  // so we can't use FunctionParams(callback->func_id) here
  auto php_params = callback->func_id->get_params();
  W << "[](";
  bool need_comma = false;
  for (int i = 1; i < fn_type->members.size(); i++) {
    if (need_comma) {
      W << ", ";
    }
    const auto *param_type = fn_type->members[i]->members[0];
    auto param = php_params[i-1].as<op_func_param>();
    W << ffi_mangled_type_string(scope_name, param_type) << " " << param->var();
    need_comma = true;
  }
  W << ") " << BEGIN;

  // to make the generated code more compact, we do not introduce tmp variables
  // for c2php-converted arguments; the same goes for the php2c result conversion
  bool is_void = tinf::get_type(callback->func_id, -1)->ptype() == tp_void;
  if (!is_void) {
    W << "return ffi_php2c(";
  }
  W << "FFI_INVOKE_CALLBACK(" << FunctionName(callback->func_id) << "(";
  bool first_arg = true;
  for (const auto &x : callback->func_id->get_params()) {
    if (!first_arg) {
      W << ", ";
    }
    auto param = x.try_as<op_func_param>();
    compile_ffi_c2php(param->var(), false, param->type_hint, W);
    first_arg = false;
  }
  W << "))";
  if (!is_void) {
    const FFIType *c_return_type = fn_type->members[0];
    W << ", ";
    compile_ffi_php2c_tag(c_return_type, scope_name, W);
    W << ")";
  }
  W << ";";

  W << NL << END;

  W << UnlockComments{};
}

void compile_ffi_func_call(VertexAdaptor<op_func_call> call, CodeGenerator &W) {
  std::string local_name = std::string(call->func_id->local_name());
  auto *scope = call->func_id->class_id->ffi_scope_mixin;
  const FFISymbol *sym = scope->find_function(local_name);

  if (!sym->type->is_signal_safe()) {
    W << "FFI_CALL(";
  }

  if (scope->is_shared_lib()) {
    W << "reinterpret_cast<" << ffi_mangled_decltype_string(scope->scope_name, sym->type) << ">(ffi_env_instance.symbols[" << sym->env_index << "].ptr)";
  } else {
    W << local_name;
  }

  W << "(";
  const auto &args = call->args();
  bool need_comma = false;
  // since FFI methods are instance methods, ignore the first $this argument
  for (int i = 1; i < args.size(); i++) {
    if (call->func_id->has_variadic_param && i == args.size() - 1) {
      auto variadic_arg = VertexUtil::get_actual_value(args[i]).try_as<op_array>();
      kphp_assert(variadic_arg);
      if (need_comma && !variadic_arg->args().empty()) {
        W << ", ";
      }
      for (int j = 0; j < variadic_arg->args().size(); j++) {
        auto arg = variadic_arg->args()[j];
        if (auto php2c_conv = arg.try_as<op_ffi_php2c_conv>()) {
          W << "ffi_vararg_php2c(" << php2c_conv->expr() << ")";
        } else {
          W << arg;
        }
        if (j != variadic_arg->args().size()-1) {
          W << ", ";
        }
      }
      break;
    }

    if (need_comma) {
      W << ", ";
    }
    if (auto as_callback = args[i].try_as<op_callback_of_builtin>()) {
      // sym->type->members[i] is Var, so its members[0] is an actual param type
      const FFIType *function_pointer = sym->type->members[i]->members[0];
      kphp_assert(function_pointer->kind == FFITypeKind::FunctionPointer);
      compile_ffi_callback(as_callback, function_pointer, scope->scope_name, W);
    } else {
      W << args[i];
    }
    need_comma = true;
  }
  W << ")";

  if (!sym->type->is_signal_safe()) {
    W << ")"; // closing FFI_CALL macro argument list
  }
}

enum class func_call_mode {
  simple = 0, // just call
  async_call, // call of resumable function inside another resumable function, but not in fork
  fork_call // call using fork
};

// to_json_impl() and from_json_impl() are represented in AST as `impl($json_encoder, ...args)`
// but we want them to be codegenerated as `f$impl(JsonEncoderTag{}, ...args)`
// here we shift call args by one and manually output cpp struct tag{} as the first argument
VertexAdaptor<op_func_call> patch_compiling_json_impl_call(CodeGenerator &W, VertexAdaptor<op_func_call> call) noexcept {
  auto args = call->args();
  auto first_arg = args.begin();
  auto v_encoder = VertexUtil::get_actual_value(*first_arg);

  std::vector<VertexPtr> rest_args;
  std::copy(++first_arg, args.end(), std::back_insert_iterator{rest_args});

  auto new_call = VertexAdaptor<op_func_call>::create(rest_args).set_location(call->location);
  new_call->str_val = call->str_val;
  new_call->func_id = call->func_id;

  W << JsonEncoderTags::get_cppStructTag_name(v_encoder->get_string()) << "{}, ";
  return new_call;
}

void compile_func_call(VertexAdaptor<op_func_call> root, CodeGenerator &W, func_call_mode mode = func_call_mode::simple) {
  if (root->func_id->is_extern()) {
    if (root->str_val == "make_clone" && tinf::get_type(root->args()[0])->is_primitive_type()) {
      // avoid generating make_clone call for primitive types such that (int, double, bool) just for beauty
      W << root->args()[0];
      return;
    }
    if (root->str_val == "kphp_tracing_func_enter_branch") {
      // we are inside a function marked with @kphp-tracing
      W << "_tr_f.enter_branch(" << root->args()[0] << ")";
      return;
    }
  }

  if (FFIRoot::is_ffi_scope_call(root)) {
    compile_ffi_func_call(root, W);
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
                                  func->as_human_readable(), func->get_resumable_path()));
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

  if (func && func->is_extern() && vk::any_of_equal(func->name, "JsonEncoder$$to_json_impl", "JsonEncoder$$from_json_impl")) {
    root = patch_compiling_json_impl_call(W, root);
  }

  auto args = root->args();
  if (func && func->cpp_variadic_call) {
    if (args.size() == 1) {
      if (auto array = VertexUtil::get_actual_value(args[0]).try_as<op_array>()) {
        args = array->args();
      }
    }
  }

  W << JoinValues(args, ", ");
  W << ")";
}

void compile_func_call_fast(VertexAdaptor<op_func_call> root, CodeGenerator &W) {
  if (!root->func_id->can_throw()) {
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

  W << ", " << ThrowAction{} << MacroEnd{};
}

void compile_ffi_cast(VertexAdaptor<op_ffi_cast> root, CodeGenerator &W) {
  const auto *from_type = tinf::get_type(root->expr());
  const auto *to_type = FFIRoot::get_ffi_type(root->php_type);
  const std::string &scope_name = FFIRoot::get_ffi_scope(root->php_type);
  if (to_type->kind == FFITypeKind::Array) {
    kphp_assert(root->has_array_size_expr() || to_type->num != -1);
    // right now only pointer types can be cast to arrays;
    // also, these arrays always have only 1 dimension (therefore, members[0] is a non-array element type)
    W << "ffi_cast_to_array" << "<" << ffi_mangled_decltype_string(scope_name, to_type->members[0]) << ">(" << root->expr() << ", "; // << root->array_size_expr() << ")";
    if (to_type->num != -1) {
      W << to_type->num;
    } else {
      W << root->array_size_expr();
    }
    W << ")";
    return;
  }
  const char *func = "ffi_cast";
  if (from_type->lookup_at_any_key()) {
    func = "ffi_cast_from_array";
  } else if (from_type->get_indirection() == 0 && ffi_builtin_type(from_type->class_type()->ffi_class_mixin->ffi_type->kind)) {
     func = "ffi_cast_scalar";
  }
  W << func << "<" << ffi_mangled_decltype_string(scope_name, to_type) << ">(" << root->expr() << ")";
}

void compile_ffi_addr(VertexAdaptor<op_ffi_addr> root, CodeGenerator &W) {
  W << "ffi_addr(" << root->expr() << ")";
}

void compile_ffi_new(VertexAdaptor<op_ffi_new> root, CodeGenerator &W) {
  const auto *ffi_type = FFIRoot::get_ffi_type(root->php_type);
  if (root->has_array_size_expr()) {
    const auto *elem_type = ffi_type->members[0];
    std::string scope_name = FFIRoot::get_ffi_scope(root->php_type);
    W << "ffi_new_cdata_array" << "<" << ffi_mangled_decltype_string(scope_name, elem_type) << ">" << "(" << root->array_size_expr() << ", " << root->owned_flag_expr() << ")";
  } else if (ffi_type->kind != FFITypeKind::Pointer) {
    W << "ffi_new_cdata<" << ffi_mangled_decltype_string(root->php_type) << ">(" << root->owned_flag_expr() << ")";
  } else {
    W << "ffi_new_cdata_ptr<" << ffi_mangled_decltype_string(root->php_type) << ">()";
  }
}

void compile_ffi_cdata_value_ref(VertexAdaptor<op_ffi_cdata_value_ref> root, CodeGenerator &W) {
  const auto *type = tinf::get_type(root->expr());
  if (type->is_ffi_ref()) {
    W << "*(" << root->expr() << ".c_value)";
  } else {
    W << root->expr() << "->c_value";
  }
}

void compile_ffi_load_call(VertexAdaptor<op_ffi_load_call> root, CodeGenerator &W) {
  auto *scope = G->get_ffi_root().find_scope(root->scope_name);

  if (!scope->is_shared_lib()) {
    compile_func_call(root->func_call(), W);
    return;
  }

  int num_dynamic_symbols = scope->variables.size() + scope->functions.size();
  if (num_dynamic_symbols == 0) {
    compile_func_call(root->func_call(), W);
    return;
  }

  W << "ffi_load_scope_symbols(";
  compile_func_call(root->func_call(), W);
  W << ", " << scope->shared_lib_id << ", " << scope->get_env_offset() << ", " << num_dynamic_symbols << ")";
}

void compile_ffi_array_get(VertexAdaptor<op_ffi_array_get> root, CodeGenerator &W) {
  W << "ffi_array_get(" << root->array() << ", " << root->key() << ")";
}

void compile_ffi_array_set(VertexAdaptor<op_ffi_array_set> root, CodeGenerator &W) {
  W << "ffi_array_set(" << root->array() << ", " << root->key() << ", " << root->value() << ")";
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
  if (func->can_throw()) {
    W << NL;
    W << "CHECK_EXCEPTION" << MacroBegin{} << ThrowAction{} << MacroEnd{};
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

  std::string xs_copy_str;
  xs_copy_str = gen_unique_name("tmp_expr");
  const TypeData *xs_type = tinf::get_type(xs);

  W << BEGIN;
  //save array to 'xs_copy_str'
  W << TypeName(xs_type) << " &" << xs_copy_str << " = " << xs << ";" << NL;

  std::string it = gen_unique_name("it");
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

  TypeData const *type_data = tinf::get_type(xs);
  if (auto xs_var = xs.try_as<op_var>()) {
    type_data = tinf::get_type(xs_var->var_id);
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
  std::string goto_name;
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

      VertexPtr val = VertexUtil::get_actual_value(expr);
      kphp_assert (val->type() == op_string);
      const std::string &s = val.as<op_string>()->str_val;
      hash = string_hash(s.c_str(), s.size());
    } else {
      cmd = v.as<op_default>()->cmd();
    }
  }
};

void compile_switch_str(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  auto cases_vertices = root->cases();
  std::vector<CaseInfo> cases(cases_vertices.size());
  std::transform(cases_vertices.begin(), cases_vertices.end(), cases.begin(), [](auto v) { return CaseInfo(v); });

  auto default_case_it = std::find_if(cases.begin(), cases.end(), vk::make_field_getter(&CaseInfo::is_default));
  auto *default_case = default_case_it != cases.end() ? &(*default_case_it) : nullptr;

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

    auto *next = cases[i].next;
    if (next && next->goto_name.empty()) {
      next->goto_name = gen_unique_name("switch_goto");
    }
  }

  auto temp_var_strval_of_condition = root->condition_on_switch();
  auto temp_var_matched_with_a_case = root->matched_with_one_case();

  W << BEGIN;
  W << temp_var_strval_of_condition << " = f$strval (" << root->condition() << ");" << NL;
  W << temp_var_matched_with_a_case << " = false;" << NL;

  W << "switch (" << temp_var_strval_of_condition << ".hash()) " << BEGIN;
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
        W << "if (!equals (" << temp_var_strval_of_condition << ", " << one_case.expr << ")) " << BEGIN;

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
  std::set<std::string> used;
  for (auto one_case : root->cases()) {
    VertexAdaptor<op_seq> cmd;
    if (auto cs = one_case.try_as<op_case>()) {
      cmd = cs->cmd();

      VertexPtr val = VertexUtil::get_actual_value(cs->expr());
      W << "case ";
      if (val->type() == op_int_const) {
        const std::string &str = val.as<op_int_const>()->str_val;
        W << str;
        kphp_error(used.insert(str).second, fmt_format("Switch: repeated cases found [{}]", str));
      } else {
        kphp_assert(VertexUtil::is_const_int(val));
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
  std::string goto_name_if_default_in_the_middle;

  auto temp_var_condition_on_switch = root->condition_on_switch();
  auto temp_var_matched_with_a_case = root->matched_with_one_case();

  W << "do " << BEGIN;
  W << temp_var_condition_on_switch << " = " << root->condition() << ";" << NL;
  W << temp_var_matched_with_a_case << " = false;" << NL;

  const auto cases = root->cases();
  const bool default_case_is_the_last = !cases.empty() && cases.back()->type() == op_default;
  for (const auto &one_case : cases) {
    VertexAdaptor<op_seq> cmd;
    if (auto cs = one_case.try_as<op_case>()) {
      cmd = cs->cmd();
      W << "if (" << temp_var_matched_with_a_case << " || eq2(" << temp_var_condition_on_switch << ", " << cs->expr() << ")) " << BEGIN;
      W << temp_var_matched_with_a_case << " = true;" << NL;
    } else {
      if (!default_case_is_the_last) {
        W << "if (" << temp_var_matched_with_a_case << ") ";
      }
      cmd = one_case.as<op_default>()->cmd();
      W << BEGIN;
      if (!default_case_is_the_last) {
        goto_name_if_default_in_the_middle = gen_unique_name("switch_goto");
        W << goto_name_if_default_in_the_middle + ": ";
      }
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
  bool has_default = false;

  for (auto one_case : root->cases()) {
    if (one_case->type() == op_default) {
      kphp_error_return(!has_default, "Switch: several `default` cases found");
      has_default = true;
      continue;
    }
  }

  if (root->kind == SwitchKind::StringSwitch) {
    compile_switch_str(root, W);
  } else if (root->kind == SwitchKind::IntSwitch) {
    compile_switch_int(root, W);
  } else if (root->kind == SwitchKind::EmptySwitch) {                         // will contain only inlined 'default' case
    W << "static_cast<void>(" << root->matched_with_one_case() << ");" << NL; // suppress g++ 'unused variable' warning
    compile_switch_var(root, W);
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
  W << "struct TracingProfilerTraits " << BEGIN
    << "static constexpr const char *file_name() noexcept { return " << RawString(location.file ? location.file->relative_file_name : "unknown file") << "; }" << NL
    << "static constexpr const char *function_name() noexcept { return " << RawString(func->as_human_readable(false)) << "; }" << NL
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
  if (func->kphp_tracing) { // append KphpTracingFuncCallGuard as a member variable also ('start()' is called below)
    TracingAutogen::codegen_runtime_func_guard_declaration(W, func);
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
  if (func->kphp_tracing) {
    TracingAutogen::codegen_runtime_func_guard_start(W, func);
  }

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
  W << "return fork_resumable(new " << FunctionClassName(func) << "(";
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

  if (func->kphp_tracing) {
    TracingAutogen::codegen_runtime_func_guard_declaration(W, func);
    TracingAutogen::codegen_runtime_func_guard_start(W, func);
  }
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
  std::string str;
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

void compile_string_build_impl(VertexAdaptor<op_string_build> root, VarName dst, const TypeData *dst_type, CodeGenerator &W) {
  struct LenArg {
    std::string var_name;
    int64_t static_len = 0;
    bool complex_flag = false;

    void compile(CodeGenerator &W) const {
      if (complex_flag) {
        W << var_name;
      } else {
        W << static_len;
      }
    }
  };
  struct AppendArg {
    const StrlenInfo &str_info;

    AppendArg(const StrlenInfo &str_info): str_info{str_info} {}

    void compile(CodeGenerator &W) const {
      if (str_info.str_flag) {
        W << RawString(str_info.str) << ", " << str_info.len;
      } else if (str_info.var_flag) {
        W << str_info.str;
      } else {
        W << str_info.v;
      }
    }
  };

  if (dst.empty() && root->args().size() <= 5) {
    bool all_string_args = true;
    for (const auto &arg : root->args()) {
      const auto *arg_type = tinf::get_type(arg);
      if (arg_type->use_optional() || vk::none_of_equal(arg_type->ptype(), tp_string, tp_tmp_string)) {
        all_string_args = false;
        break;
      }
    }
    // we generate a 1-argument string build for some optimized constructions
    // like sprintf('foo') will become a string build of 'foo' argument;
    // mostly to make the generated code prettier
    if (all_string_args && root->args().size() == 1) {
      W << root->args()[0];
      return;
    }
    // we have specialized str_concat() routines for up to 5 arguments
    if (all_string_args && root->args().size() <= 5) {
      W << "str_concat(" << JoinValues(root->args(), ", ") << ")";
      return;
    }
  }

  std::vector<StrlenInfo> info(root->size());
  bool ok = true;
  bool was_dynamic = false;
  bool was_object = false;
  LenArg len_arg;
  int ii = 0;
  for (auto i : root->args()) {
    info[ii].v = i;
    VertexPtr value = VertexUtil::get_actual_value(i);
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
      len_arg.static_len += info[ii].len;
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
        len_arg.static_len += value_length;
      }

      info[ii].len = value_length;
    }

    ii++;
  }
  if (!ok) {
    return;
  }

  bool complex_flag = was_dynamic || was_object;
  len_arg.complex_flag = complex_flag;
  std::string tmp_string_name;
  int n_next_var_idx = 0;
  if (complex_flag) {
    W << "(" << BEGIN;
    std::vector<std::string> to_add;
    for (auto &str_info : info) {
      if (str_info.str_flag) {
        continue;
      }
      if (str_info.len == STRLEN_DYNAMIC || str_info.len == STRLEN_OBJECT) {
        std::string var_name = "tmp_var" + std::to_string(n_next_var_idx++);

        if (str_info.len == STRLEN_DYNAMIC) {
          TmpExpr v{str_info.v};
          bool can_save_ref_flag = can_save_ref(str_info.v);
          W << "const " << TypeName(v.get_type()) << " " <<
            (can_save_ref_flag ? "&" : "") <<
            var_name << "=" << v << ";" << NL;
        } else if (str_info.len == STRLEN_OBJECT) {
          W << "const string " << var_name << " = f$strval" <<
            "(" << str_info.v << ");" << NL;
        }

        to_add.push_back(var_name);
        str_info.var_flag = true;
        str_info.str = var_name;
      }
    }

    len_arg.var_name = "tmp_strlen";
    W << "string::size_type " << len_arg.var_name << " = " << len_arg.static_len;
    for (const auto &str : to_add) {
      W << " + max_string_size (" << str << ")";
    }
    W << ";" << NL;
  }

  if (!dst.empty()) {
    if (!complex_flag) { // open the block if it's not opened yet
      W << "(" << BEGIN;
    }
    // TODO: handle mixed too (it's more tricky: sometimes we need to convert mixed to string, etc.)
    kphp_assert(dst_type->ptype() == tp_string);
    VarName string_dst = dst;
    if (dst_type->ptype() == tp_string && dst_type->use_optional()) {
      W << "auto &as_string_ = " << dst << ".ref();" << NL;
      string_dst = VarName{"as_string_"};
    }
    // reserve_at_least will handle the shared string by cloning it,
    // so we can use append_unsafe+finish_append safely here;
    // the unsafe append method doesn't add a trailing 0 byte after each append,
    // it also doesn't check whether we need to reserve more memory for the result;
    // the finish_append call will insert that 0 byte once in the very end
    W << string_dst << ".reserve_at_least(" << len_arg << " + " << string_dst << ".size());" << NL;
    W << string_dst;
    for (const auto &str_info : info) {
      W << ".append_unsafe(" << AppendArg{str_info} << ")";
    }
    W << ".finish_append();" << NL;
    W << END << ")";
    return;
  }

  if (complex_flag) {
    tmp_string_name = "tmp_string_";
    W << "string " << tmp_string_name << " = ";
  }
  W << "string (" << len_arg << ", true)";
  for (const auto &str_info : info) {
    W << ".append_unsafe (" << AppendArg{str_info} << ")";
  }
  W << ".finish_append()";

  if (complex_flag) {
    W << ";" << NL
      << tmp_string_name << ";" << NL
      << END << ")";
  }
}

void compile_string_build_as_string(VertexAdaptor<op_string_build> root, CodeGenerator &W) {
  compile_string_build_impl(root, {}, nullptr, W);
}

bool try_compile_append_inplace(VertexAdaptor<op_set_dot> root, CodeGenerator &W) {
  if (root->rhs()->type() == op_string_build) {
    const auto *lhs_type = tinf::get_type(root->lhs());
    if (lhs_type->ptype() != tp_string) {
      return false;
    }
    // compile `$lhs .= $s1 . $s2 . [...] . $s3` more efficiently:
    // append all strings directly to the $lhs without creating a temporary
    // string for the $rhs result; also, grow $lhs accordingly, so it
    // can fit all the string parts
    if (root->lhs()->type() == op_var) {
      kphp_assert(tinf::get_type(root->lhs().as<op_var>())->ptype() == tp_string);
      compile_string_build_impl(root->rhs().as<op_string_build>(), VarName{root->lhs().as<op_var>()->var_id}, lhs_type, W);
      return true;
    }
    W << "(" << BEGIN;
    W << "auto &concat_lhs_ = " << root->lhs() << ";" << NL;
    compile_string_build_impl(root->rhs().as<op_string_build>(), VarName{"concat_lhs_"}, lhs_type, W);
    W << ";" << NL << END << ")";
    return true;
  }

  return false;
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
      W << ", " << precomputed_hash << "_i64";
    }
    W << ")";
  }
}

void compile_index_of_string(VertexAdaptor<op_index> root, CodeGenerator &W) {
  kphp_assert(root->has_key());

  W << root->array() << ".get_value(" << root->key() << ")";
}

void compile_instance_prop(VertexAdaptor<op_instance_prop> root, CodeGenerator &W) {
  switch (root->access_type) {
    case InstancePropAccessType::Default:
      W << root->instance() << "->$" << root->get_string();
      break;

    case InstancePropAccessType::CData: {
      const auto *type = tinf::get_type(root->instance());
      if (type->is_ffi_ref() || type->get_indirection() != 0) {
        // CDataPtr or CDataRef
        W << root->instance() << ".c_value->" << root->get_string();
      } else {
        W << root->instance() << "->c_value." << root->get_string();
      }
      break;
    }

    case InstancePropAccessType::CDataDirect:
      W << root->instance() << "." << root->get_string();
      break;

    case InstancePropAccessType::CDataDirectPtr:
      W << root->instance() << "->" << root->get_string();
      break;

    case InstancePropAccessType::ExternVar: {
      auto *scope_class = tinf::get_type(root->instance())->class_type()->ffi_scope_mixin;
      if (scope_class->is_shared_lib()) {
        const auto *sym = scope_class->find_variable(root->get_string());
        W << "*reinterpret_cast<" << ffi_mangled_decltype_string(scope_class->scope_name, sym->type) << "*>(ffi_env_instance.symbols[" << sym->env_index << "].ptr)";
      } else {
        W << root->get_string();
      }
      break;
    }
  }
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
      W << index->key();
      if (auto precomputed_hash = can_use_precomputed_hash_indexing_array(index->key())) {
        W << ", " << precomputed_hash << "_i64";
      }
      W << ")";
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

  for (auto x : list) {
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
      VertexPtr key = VertexUtil::get_actual_value(arrow->key());
      if (!has_double_arrow && key->type() == op_int_const) {
        if (key.as<op_int_const>()->str_val == std::to_string(key_id)) {
          root->args()[key_id] = arrow->value();
          int_cnt++;
          continue;
        }
      }
      has_double_arrow = true;
      PrimitiveType tp = tinf::get_type(key)->ptype();
      if (tp == tp_int) {
        int_cnt++;
      } else {
        if (tp == tp_string && key->type() == op_string) {
          const std::string &key_str = key.as<op_string>()->str_val;
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

  std::string arr_name = "tmp_array";
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
        W << ", " << precomputed_hash << "_i64";
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
    const std::string &key_str = VertexUtil::get_actual_value(double_arrow.as<op_double_arrow>()->lhs())->get_string();
    sorted_by_hash.emplace_back(string_hash(key_str.c_str(), key_str.size()), double_arrow.as<op_double_arrow>()->rhs());
  }
  std::sort(sorted_by_hash.begin(), sorted_by_hash.end(), [](const auto &a, const auto &b) -> bool {
    kphp_assert(a.second == b.second || a.first != b.first);
    return a.first < b.first;
  });

  const auto val_gen = [](CodeGenerator &W, const std::pair<int64_t, VertexPtr> &hash_and_rhs) {
    W << hash_and_rhs.second;
  };

  const char *sep = W.get_context().inside_macro ? " COMMA " : ", ";
  W << TypeName(tinf::get_type(root)) << "{shape_variadic_constructor_stub{} " << sep <<
    JoinValues(sorted_by_hash, sep, join_mode::one_line, val_gen) << "}";
}

void compile_callback_of_builtin(VertexAdaptor<op_callback_of_builtin> root, CodeGenerator &W) {
  /**
   * PHP code like this:
   *   array_map(fn($x) => $x + $extern, [1,2,3]);
   * Will be transformed to:
   *   array_map([captured1 = $extern] (auto &&... args) {
   *       return lambda$xxx(captured1, std::forward<decltype(args)>(args)...);
   *   }), const_array);
   * Where the body calls a lambda function:
   *    int lambda$xxx(int $extern, int $x) { return $xx + $extern; }
   * Captured vars are not always op_var. For example, [captured1 = check_not_false($extern).val()] after smart casts.
   */
  W << LockComments{};

  int idx = 0;
  W << "[" << JoinValues(*root, ", ", join_mode::one_line, [&idx](CodeGenerator &W, VertexPtr cpp_captured) {
    W << "captured" << (++idx) << " = " << cpp_captured;
  }) << "]";

  FunctionSignatureGenerator(W) << "(auto &&... args) " << BEGIN;

  W << "return " << FunctionName(root->func_id) << "(";
  for (int idx = 1; idx <= root->size(); ++idx) {
    W << "captured" << idx << ", ";
  }
  W << "std::forward<decltype(args)>(args)...);";

  W << NL << END;
  W << UnlockComments{};
}

void compile_defined(VertexPtr root __attribute__((unused)), CodeGenerator &W __attribute__((unused))) {
  W << "false";
  //TODO: it is not CodeGen part
}

void compile_safe_version(VertexPtr root, CodeGenerator &W) {
  if (auto set_value = root.try_as<op_set_value>()) {
    TmpExpr key{set_value->key()};
    TmpExpr value{set_value->value()};
    W << "SAFE_SET_VALUE " << MacroBegin{} <<
      set_value->array() << ", " <<
      key << ", " <<
      TypeName(key.get_type()) << ", " <<
      value << ", " <<
      TypeName(value.get_type()) <<
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
    TmpExpr rhs{op->rhs()};
    W << op->lhs() << ", " <<
      OpInfo::str(root->type()) << ", " <<
      rhs << ", " <<
      TypeName(rhs.get_type()) <<
      MacroEnd{};
  } else if (auto pb = root.try_as<op_push_back>()) {
    TmpExpr value{pb->value()};
    W << "SAFE_PUSH_BACK " << MacroBegin{} <<
      pb->array() << ", " <<
      value << ", " <<
      TypeName(value.get_type()) <<
      MacroEnd{};
  } else if (auto pb = root.try_as<op_push_back_return>()) {
    TmpExpr value{pb->value()};
    W << "SAFE_PUSH_BACK_RETURN " << MacroBegin{} <<
      pb->array() << ", " <<
      value << ", " <<
      TypeName(value.get_type()) <<
      MacroEnd{};
  } else if (root->type() == op_array) {
    compile_array(root.as<op_array>(), W);
    return;
  } else if (auto index = root.try_as<op_index>()) {
    kphp_assert (index->has_key());
    TmpExpr key{index->key()};
    W << "SAFE_INDEX " << MacroBegin{} <<
      index->array() << ", " <<
      key << ", " <<
      TypeName(key.get_type()) <<
      MacroEnd{};
  } else {
    kphp_error (0, fmt_format("Safe version of [{}] is not supported", OpInfo::str(root->type())));
    kphp_fail();
  }

}


void compile_set_value(VertexAdaptor<op_set_value> root, CodeGenerator &W) {
  W << "(" << root->array() << ").set_value (" << root->key() << ", " << root->value();
  if (auto precomputed_hash = can_use_precomputed_hash_indexing_array(root->key())) {
    W << ", " << precomputed_hash << "_i64";
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
      fun_name = cur_fun->as_human_readable();
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
  std::string str;
  switch (tp) {
    case op_seq:
      W << BEGIN << AsSeq{root.as<op_seq>()} << END;
      break;
    case op_seq_rval:
      compile_seq_rval(root, W);
      break;

    case op_int_const:
      W << IntLit{root.as<op_int_const>()->str_val};
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
    case op_ffi_cdata_value_ref:
      compile_ffi_cdata_value_ref(root.as<op_ffi_cdata_value_ref>(), W);
      break;
    case op_ffi_new:
      compile_ffi_new(root.as<op_ffi_new>(), W);
      break;
    case op_ffi_addr:
      compile_ffi_addr(root.as<op_ffi_addr>(), W);
      break;
    case op_ffi_cast:
      compile_ffi_cast(root.as<op_ffi_cast>(), W);
      break;
    case op_ffi_load_call:
      compile_ffi_load_call(root.as<op_ffi_load_call>(), W);
      break;
    case op_ffi_array_get:
      compile_ffi_array_get(root.as<op_ffi_array_get>(), W);
      break;
    case op_ffi_array_set:
      compile_ffi_array_set(root.as<op_ffi_array_set>(), W);
      break;
    case op_func_call:
      compile_func_call_fast(root.as<op_func_call>(), W);
      break;
    case op_callback_of_builtin:
      compile_callback_of_builtin(root.as<op_callback_of_builtin>(), W);
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
      const auto *tp = tinf::get_type(root);
      if (auto klass = tp->class_type()) {
        if (klass->is_class()) {
          W << root.as<op_clone>()->expr() << ".clone()";
          break;
        }
        if (klass->is_ffi_cdata()) {
          W << "ffi_clone(" << root.as<op_clone>()->expr() << ")";
          break;
        }
      }
      kphp_error_return(false, "unsupported operand for cloning");
      break;
    }
    case op_alloc: {
      const TypeData *tp = tinf::get_type(root);
      kphp_assert(tp->ptype() == tp_Class);
      const auto *alloc_function = tp->class_type()->is_empty_class() ? "().empty_alloc()" : "().alloc()";
      W << TypeName(tp) << alloc_function;
      break;
    }
    default:
      kphp_fail();
      break;
  }
}

} // anonymous namespace

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
