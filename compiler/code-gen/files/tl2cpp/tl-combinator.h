// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

namespace tl2cpp {
enum class CombinatorPart { LEFT, RIGHT };

std::vector<std::string> get_not_optional_fields_masks(const vk::tlo_parsing::combinator *constructor);

struct CombinatorGen {
  const vk::tlo_parsing::combinator *combinator;
  CombinatorPart part;
  bool typed_mode;
  std::string var_num_access;

  explicit CombinatorGen(const vk::tlo_parsing::combinator *combinator, CombinatorPart part, bool typed_mode);

  void compile(CodeGenerator &W) const;

  void compile_left(CodeGenerator &W) const;

  void compile_right(CodeGenerator &W) const;

  virtual void gen_before_args_processing(CodeGenerator &W __attribute__((unused))) const {};
  virtual void gen_arg_processing(CodeGenerator &W, const std::unique_ptr<vk::tlo_parsing::arg> &arg) const = 0;
  virtual void gen_after_args_processing(CodeGenerator &W __attribute__((unused))) const {};

  virtual void gen_result_expr_processing(CodeGenerator &W) const = 0;
};

/* The code that is common for combinators (func/constructor) store method generation.
 *
 * 1) Field masks handling:
    void c_hints_objectExt::store(const mixed& tl_object, int fields_mask) {
      (void)tl_object;
      t_Int().store(tl_arr_get(tl_object, tl_str$type, 2, -445613708));
      t_Int().store(tl_arr_get(tl_object, tl_str$object_id, 3, 65801733));
      if (fields_mask & (1 << 0)) {
        t_Double().store(tl_arr_get(tl_object, tl_str$rating, 4, -1130913585));
      }
      if (fields_mask & (1 << 1)) {
        t_String().store(tl_arr_get(tl_object, tl_str$text, 5, -193436300));
      }
    }
 * 2) Exclamation mark handling (! modifier):
    std::unique_ptr<tl_func_base> f_rpcProxy_diagonalTargets::store(const mixed& tl_object) {
      auto tl_func_state = make_unique_on_script_memory<f_rpcProxy_diagonalTargets>();
      (void)tl_object;
      f$store_int(0xee090e42);
      t_Int().store(tl_arr_get(tl_object, tl_str$offset, 2, -1913876069));
      t_Int().store(tl_arr_get(tl_object, tl_str$limit, 3, 492966325));
      auto _cur_arg = tl_arr_get(tl_object, tl_str$query, 4, 1563700686);
      string target_f_name = tl_arr_get(_cur_arg, tl_str$_, 0, -2147483553).as_string();
      if (!tl_storers_ht.has_key(target_f_name)) {
        CurrentProcessingQuery::get().raise_storing_error("Function %s not found in tl-scheme", target_f_name.c_str());
        return {};
      }
      const auto &storer_kv = tl_storers_ht.get_value(target_f_name);
      tl_func_state->X.fetcher = storer_kv(_cur_arg);
      return std::move(tl_func_state);
    }
 * 3) Handling of the main part of the type expression in TypeExprStore/Fetch
*/
struct CombinatorStore : CombinatorGen {
  CombinatorStore(const vk::tlo_parsing::combinator *combinator, CombinatorPart part, bool typed_mode)
    : CombinatorGen(combinator, part, typed_mode){};

  void gen_before_args_processing(CodeGenerator &W) const final;

  void gen_arg_processing(CodeGenerator &W, const std::unique_ptr<vk::tlo_parsing::arg> &arg) const final;

  void gen_result_expr_processing(CodeGenerator &W) const final;

private:
  static std::string get_value_absence_check_for_optional_arg(const std::unique_ptr<vk::tlo_parsing::arg> &arg);
};

/* The code that is common for combinators (func/constructor) fetch method generation.
 * 1) Field masks handling:
    array<mixed> c_hints_objectExt::fetch(int fields_mask) {
      array<mixed> result;
      result.set_value(tl_str$type, t_Int().fetch(), -445613708);
      result.set_value(tl_str$object_id, t_Int().fetch(), 65801733);
      if (fields_mask & (1 << 0)) {
        result.set_value(tl_str$rating, t_Double().fetch(), -1130913585);
      }
      if (fields_mask & (1 << 1)) {
        result.set_value(tl_str$text, t_String().fetch(), -193436300);
      }
      return result;
    }
 * 2) Exclamation mark handling (! modifier):
    var f_rpcProxy_diagonalTargets::fetch() {
      fetch_magic_if_not_bare(0x1cb5c415, "Incorrect magic in result of function: rpcProxy.diagonalTargets");
      return t_Vector<t_Vector<t_Maybe<tl_exclamation_fetch_wrapper, 0>, 0>, 0>(t_Vector<t_Maybe<tl_exclamation_fetch_wrapper, 0>,
 0>(t_Maybe<tl_exclamation_fetch_wrapper, 0>(std::move(X)))).fetch();
    }
 * 3) Handling of the main part of the type expression in TypeExprStore/Fetch
*/
struct CombinatorFetch : CombinatorGen {
  CombinatorFetch(const vk::tlo_parsing::combinator *combinator, CombinatorPart part, bool typed_mode)
    : CombinatorGen(combinator, part, typed_mode){};

  void gen_before_args_processing(CodeGenerator &W) const final;

  void gen_arg_processing(CodeGenerator &W, const std::unique_ptr<vk::tlo_parsing::arg> &arg) const final;

  void gen_after_args_processing(CodeGenerator &W) const final;

  void gen_result_expr_processing(CodeGenerator &W) const final;
};
} // namespace tl2cpp
