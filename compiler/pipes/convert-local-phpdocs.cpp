#include "convert-local-phpdocs.h"

#include "compiler/data/var-data.h"
#include "compiler/phpdoc.h"

/*
 * Цель этого пайпа — распарсить phpdoc'и внутри функций (т.е. НЕ НАД функциями/полями, а именно внутри):
 * найти @var $v type
 * Т.е. превратить op_phpdoc_raw в один или несколько op_phpdoc_var
 * Это можно делать лишь сейчас, после register variables и знания обо всех классах
 */
VertexPtr ConvertLocalPhpdocsPass::visit_phpdoc_and_extract_vars(VertexAdaptor<op_phpdoc_raw> phpdoc_raw) {
  VertexPtr result;

  // имея @var $v type, делаем op_phpdoc_var с нужным type_rule и var_id
  auto create_var_vertex = [](const std::vector<VarPtr> &lookup_vars_list, const PhpDocTagParseResult &tag_var) {
    auto var_it = std::find_if(lookup_vars_list.begin(), lookup_vars_list.end(),
                               [&tag_var](VarPtr var) {
                                 return var->name == tag_var.var_name;
                               });
    if (var_it == lookup_vars_list.end()) {
      return VertexPtr{};
    }
    auto inner_var = VertexAdaptor<op_var>::create().set_location(stage::get_location());
    inner_var->type_rule = VertexAdaptor<op_set_check_type_rule>::create(tag_var.type_expr);
    inner_var->var_id = *var_it;
    inner_var->str_val = inner_var->var_id->name;
    return (VertexPtr)VertexAdaptor<op_phpdoc_var>::create(inner_var).set_location(stage::get_location());
  };

  // в одном /** op_phpdoc_raw */ может быть несколько @var'ов (например, перед list() такое пишут)
  for (auto &tag_var : phpdoc_find_tag_multi(phpdoc_raw->phpdoc_str, php_doc_tag::var, current_function)) {
    // если /** @var int */ $v ... — понимаем, что речь идёт про $v
    if (tag_var.var_name.empty()) {
      tag_var.var_name = phpdoc_raw->next_var_name;
    }
    kphp_error_act(!tag_var.var_name.empty(), "@var with empty var name", continue);

    // @var $v — она может быть локальной, static внутри функции или глобальной вообще
    VertexPtr phpdoc_var = create_var_vertex(current_function->local_var_ids, tag_var);
    if (!phpdoc_var) {
      phpdoc_var = create_var_vertex(current_function->static_var_ids, tag_var);
    }
    if (!phpdoc_var) {
      phpdoc_var = create_var_vertex(current_function->global_var_ids, tag_var);
    }
    // если не нашлось — ошибочный @var, ошибка компиляции
    if (!phpdoc_var) {
      bool is_param_var = static_cast<bool>(create_var_vertex(current_function->param_ids, tag_var));
      kphp_error(!is_param_var, "Do not use @var for arguments, use @param or type declaration");
      kphp_error( is_param_var, fmt_format("Unknown var ${} in phpdoc", tag_var.var_name));
      continue;
    }

    // если несколько @var внутри /** op_phpdoc_raw */, делаем op_seq
    result = result ? (VertexPtr)VertexAdaptor<op_seq>::create(result, phpdoc_var) : phpdoc_var;
  }

  return result ?: (VertexPtr)VertexAdaptor<op_empty>::create();
}

VertexPtr ConvertLocalPhpdocsPass::on_enter_vertex(VertexPtr root, LocalT *) {
  if (auto phpdoc_raw = root.try_as<op_phpdoc_raw>()) {
    return visit_phpdoc_and_extract_vars(phpdoc_raw);
    // после этого пайпа op_phpdoc_raw уже нет в AST:
    // вместо них есть op_phpdoc_var с корректными var_id и type_rule
  }

  return root;
}
