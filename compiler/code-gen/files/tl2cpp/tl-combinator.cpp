#include "compiler/code-gen/files/tl2cpp/tl-combinator.h"

#include "compiler/code-gen/files/tl2cpp/tl-type-expr.h"

namespace tl2cpp {
inline std::vector<std::string> get_not_optional_fields_masks(const vk::tl::combinator *constructor) {
  std::vector<std::string> res;
  for (const auto &arg : constructor->args) {
    if (arg->var_num != -1 && type_of(arg->type_expr)->is_integer_variable() && !arg->is_optional()) {
      res.emplace_back(arg->name);
    }
  }
  return res;
}

CombinatorGen::CombinatorGen(const vk::tl::combinator *combinator, CombinatorPart part, bool typed_mode) :
  combinator(combinator),
  part(part),
  typed_mode(typed_mode) {
  kphp_error(!(part == CombinatorPart::RIGHT && combinator->is_constructor()), "Storing/fetching of constructor right part is never needed and strange");
  this->var_num_access = (combinator->is_function() && part == CombinatorPart::LEFT ? "tl_func_state->" : "");
}

void CombinatorGen::compile(CodeGenerator &W) const {
  switch (part) {
    case CombinatorPart::LEFT:
      compile_left(W);
      break;
    case CombinatorPart::RIGHT:
      kphp_assert(!combinator->is_constructor());
      compile_right(W);
      break;
  }
}

void CombinatorGen::compile_left(CodeGenerator &W) const {
  tl2cpp::cur_combinator = combinator;
  // Создаем локальные переменные для хранения филд масок
  if (combinator->is_constructor()) {
    auto fields_masks = tl2cpp::get_not_optional_fields_masks(combinator);
    for (const auto &item : fields_masks) {
      W << "int " << item << "{0}; (void) " << item << ";" << NL;
    }
  }
  gen_before_args_processing(W);
  for (const auto &arg : combinator->args) {
    if (arg->is_optional()) {
      continue;
    }
    gen_arg_processing(W, arg);
  }
  gen_after_args_processing(W);
}

void CombinatorGen::compile_right(CodeGenerator &W) const {
  gen_result_expr_processing(W);
}

void CombinatorStore::gen_before_args_processing(CodeGenerator &W) const {
  W << "(void)tl_object;" << NL;
  if (combinator->is_function()) {
    W << fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(combinator->id)) << NL;
  }
}

void CombinatorStore::gen_arg_processing(CodeGenerator &W, const std::unique_ptr<vk::tl::arg> &arg) const {
  if (arg->is_named_fields_mask_bit()) {
    return;
  }
  if (arg->is_fields_mask_optional()) {
    W << fmt_format("if ({}{} & (1 << {})) ", var_num_access,
                    combinator->get_var_num_arg(arg->exist_var_num)->name,
                    arg->exist_var_bit) << BEGIN;
    if (typed_mode) {
      // Проверяем, что не забыли проставить поле под филд маской
      std::string value_check = get_value_absence_check_for_optional_arg(arg);
      if (!value_check.empty()) {
        W << "if (" << value_check << ") " << BEGIN;
        W
          << fmt_format(R"(CurrentProcessingQuery::get().raise_storing_error("Optional field %s of %s is not set, but corresponding fields mask bit is set", "{}", "{}");)",
                        arg->name, combinator->name) << NL;
        W << "return" << (combinator->is_function() ? " {};" : ";") << NL;
        W << END << NL;
      }
    }
  }
  if (!arg->is_forwarded_function()) {
    W << tl2cpp::TypeExprStore(arg, var_num_access, typed_mode);
  }
  //Обработка восклицательного знака
  if (arg->is_forwarded_function()) {
    kphp_assert(combinator->is_function());
    auto as_type_var = arg->type_expr->as<vk::tl::type_var>();
    kphp_assert(as_type_var);
    if (!typed_mode) {
      W << "auto _cur_arg = "
        << fmt_format("tl_arr_get(tl_object, {}, {}, {})", tl2cpp::register_tl_const_str(arg->name), arg->idx, tl2cpp::hash_tl_const_str(arg->name))
        << ";" << NL;
      W << "string target_f_name = "
        << fmt_format("tl_arr_get(_cur_arg, {}, 0, {}).as_string()", tl2cpp::register_tl_const_str("_"), tl2cpp::hash_tl_const_str("_"))
        << ";" << NL;
      W << "if (!tl_storers_ht.has_key(target_f_name)) " << BEGIN
        << "CurrentProcessingQuery::get().raise_storing_error(\"Function %s not found in tl-scheme\", target_f_name.c_str());" << NL
        << "return {};" << NL
        << END << NL;
      W << "const auto &storer_kv = tl_storers_ht.get_value(target_f_name);" << NL;
      W << "tl_func_state->" << combinator->get_var_num_arg(as_type_var->var_num)->name << ".fetcher = storer_kv(_cur_arg);" << NL;
    } else {
      W << "if (tl_object->$" << arg->name << ".is_null()) " << BEGIN
        << R"(CurrentProcessingQuery::get().raise_storing_error("Field \")" << arg->name << R"(\" not found in tl object");)" << NL
        << "return {};" << NL
        << END << NL;
      W << "tl_func_state->" << combinator->get_var_num_arg(as_type_var->var_num)->name << ".fetcher = "
        << tl2cpp::get_tl_object_field_access(arg, tl2cpp::field_rw_type::READ) << ".get()->store();" << NL;
    }
  } else if (arg->var_num != -1 && tl2cpp::type_of(arg->type_expr)->is_integer_variable()) {
    // Запоминаем филд маску для последующего использования
    // Может быть либо локальной переменной либо полем структуры
    if (!typed_mode) {
      W << fmt_format("{}{} = tl_arr_get(tl_object, {}, {}, {}).to_int();",
                      var_num_access,
                      combinator->get_var_num_arg(arg->var_num)->name,
                      tl2cpp::register_tl_const_str(arg->name),
                      arg->idx,
                      tl2cpp::hash_tl_const_str(arg->name)) << NL;
    } else {
      W << var_num_access << combinator->get_var_num_arg(arg->var_num)->name << " = " << tl2cpp::get_tl_object_field_access(arg, tl2cpp::field_rw_type::READ)
        << ";" << NL;
    }
  }
  if (arg->is_fields_mask_optional()) {
    W << END << NL;
  }
}

void CombinatorStore::gen_result_expr_processing(CodeGenerator &W) const {
  kphp_assert(typed_mode);
  if (!combinator->original_result_constructor_id) {
    const auto &magic_storing = tl2cpp::get_magic_storing(combinator->result.get());
    if (!magic_storing.
                        empty()
      ) {
      W << magic_storing <<
        NL;
    }
  } else {
    W << fmt_format("f$store_int({:#010x});", combinator->original_result_constructor_id) <<
      NL;
  }
  W <<
    tl2cpp::get_full_value(combinator
                             ->result.
      get(), var_num_access
    ) << ".typed_store(tl_object->$value);" <<
    NL;
}

std::string CombinatorStore::get_value_absence_check_for_optional_arg(const std::unique_ptr<vk::tl::arg> &arg) {
  kphp_assert(arg->is_fields_mask_optional());
  auto type = tl2cpp::type_of(arg->type_expr);
  std::string check_target = "tl_object->$" + arg->name;
  if (tl2cpp::is_tl_type_wrapped_to_Optional(type) || type->id == TL_LONG || !tl2cpp::CUSTOM_IMPL_TYPES.count(type->name)) {
    // Если это Optional ИЛИ var ИЛИ class_instance<T>
    return check_target + ".is_null()";
  } else {
    // Иначе это Optional, который дополнительно не оборачивается в Optional под филд маской,
    // поэтому мы не можем отличить записанное значение Maybe false, от отсутстсвия значения
    return "";
  }
}

void CombinatorFetch::gen_before_args_processing(CodeGenerator &W) const {
  if (!typed_mode) {
    W << "array<var> result;" << NL;
  }
};

void CombinatorFetch::gen_arg_processing(CodeGenerator &W, const std::unique_ptr<vk::tl::arg> &arg) const {
  if (arg->is_fields_mask_optional()) {
    W << fmt_format("if ({}{} & (1 << {})) ", var_num_access,
                    combinator->get_var_num_arg(arg->exist_var_num)->name,
                    arg->exist_var_bit) << BEGIN;
  }
  W << tl2cpp::TypeExprFetch(arg, var_num_access, typed_mode);
  if (arg->var_num != -1 && tl2cpp::type_of(arg->type_expr)->is_integer_variable()) {
    // запоминаем филд маску для дальнейшего использования
    if (!typed_mode) {
      W << var_num_access << combinator->get_var_num_arg(arg->var_num)->name << " = result.get_value(" << tl2cpp::register_tl_const_str(arg->name) << ", "
        << tl2cpp::hash_tl_const_str(arg->name) << ").to_int();" << NL;
    } else {
      W << var_num_access << combinator->get_var_num_arg(arg->var_num)->name << " = " << tl2cpp::get_tl_object_field_access(arg, tl2cpp::field_rw_type::READ)
        << ";" << NL;
    }
  }
  if (arg->is_fields_mask_optional()) {
    W << END << NL;
  }
}

void CombinatorFetch::gen_after_args_processing(CodeGenerator &W) const {
  if (!typed_mode) {
    W << "return result;" << NL;
  }
};

void CombinatorFetch::gen_result_expr_processing(CodeGenerator &W) const {
  // Если функция возвращает флатящийся тип, то после perform_flat_optimization() вместо него подставляется его внутренность.
  // Но при фетчинге функции нам всегда нужен мэджик оригинального типа, даже если он флатится.
  // Для этого было введено поле original_result_constructor_id у функций, в котором хранится мэджик реального типа, если результат флатится, и 0 иначе.
  if (!combinator->original_result_constructor_id) {
    const auto &magic_fetching = tl2cpp::get_magic_fetching(combinator->result.get(),
                                                            fmt_format("Incorrect magic in result of function: {}", tl2cpp::cur_combinator->name));
    if (!magic_fetching.empty()) {
      W << magic_fetching << NL;
    }
  } else {
    W << fmt_format(R"(fetch_magic_if_not_bare({:#010x}, "Incorrect magic in result of function: {}");)",
                    static_cast<uint32_t>(combinator->original_result_constructor_id), tl2cpp::cur_combinator->name)
      << NL;
  }
  if (!typed_mode) {
    W << "return " << tl2cpp::get_full_value(combinator->result.get(), "") << ".fetch();" << NL;
    return;
  }
  if (auto type_var = combinator->result->as<vk::tl::type_var>()) {
    // multiexclamation оптимизация
    W << "return " << tl2cpp::cur_combinator->get_var_num_arg(type_var->var_num)->name << ".fetcher->typed_fetch();" << NL;
    return;
  }
  // для любой getChatInfo implements RpcFunction есть getChatInfo_result implements RpcFunctionReturnResult
  W << tl2cpp::get_php_runtime_type(combinator, true, combinator->name + "_result") << " result;" << NL
    << "result.alloc();" << NL
    << tl2cpp::get_full_value(combinator->result.get(), "") + ".typed_fetch_to(result->$value);" << NL
    << "return result;" << NL;
}
}
