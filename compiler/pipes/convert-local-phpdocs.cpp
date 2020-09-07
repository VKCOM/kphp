#include "compiler/pipes/convert-local-phpdocs.h"

#include "compiler/data/var-data.h"
#include "compiler/phpdoc.h"

// This pipe parses phpdoc comments inside functions (emphasis on *inside*).
// It finds function-local /** @var $v type */ comments (op_phpdoc_raw)
// and turns them into one or more op_phpdoc_var.
// This pipe needs a knowledge about all classes and can't be run before register variables.
VertexPtr ConvertLocalPhpdocsPass::visit_phpdoc_and_extract_vars(VertexAdaptor<op_phpdoc_raw> phpdoc_raw) {
  VertexPtr result;

  // turn @var $v type into op_phpdoc_var with type_rule and var_id
  auto create_var_vertex = [](const std::vector<VarPtr> &lookup_vars_list, const PhpDocTagParseResult &tag_var) {
    auto var_it = std::find_if(lookup_vars_list.begin(), lookup_vars_list.end(),
                               [&tag_var](VarPtr var) {
                                 return var->name == tag_var.var_name;
                               });
    if (var_it == lookup_vars_list.end()) {
      return VertexPtr{};
    }
    // op_phpdoc_var consists of only one child (op_var) with var_id and type_rule for tinf;
    // this approach makes it automagically count as usage in cfg and collect edges
    auto inner_var = VertexAdaptor<op_var>::create().set_location(stage::get_location());
    inner_var->type_rule = VertexAdaptor<op_set_check_type_rule>::create(tag_var.type_expr);
    inner_var->var_id = *var_it;
    inner_var->str_val = inner_var->var_id->name;
    return (VertexPtr)VertexAdaptor<op_phpdoc_var>::create(inner_var).set_location(stage::get_location());
  };

  // one op_phpdoc_raw can contain several @var directives;
  // a common example is a block of @var comments above the list() assignment
  for (auto &tag_var : phpdoc_find_tag_multi(phpdoc_raw->phpdoc_str, php_doc_tag::var, current_function)) {
    // /** @var int */ $v ... - assume that @var refers to $v
    if (tag_var.var_name.empty()) {
      tag_var.var_name = phpdoc_raw->next_var_name;
    }
    kphp_error_act(!tag_var.var_name.empty(), "@var with empty var name", continue);

    // @var $v can refer to a local, static (inside a function) or global variable
    VertexPtr phpdoc_var = create_var_vertex(current_function->local_var_ids, tag_var);
    if (!phpdoc_var) {
      phpdoc_var = create_var_vertex(current_function->static_var_ids, tag_var);
    }
    if (!phpdoc_var) {
      phpdoc_var = create_var_vertex(current_function->global_var_ids, tag_var);
    }
    // if we can't find a variable that @var refers to - it's a compile-time error
    if (!phpdoc_var) {
      bool is_param_var = static_cast<bool>(create_var_vertex(current_function->param_ids, tag_var));
      kphp_error(!is_param_var, "Do not use @var for arguments, use @param or type declaration");
      kphp_error( is_param_var, fmt_format("Unknown var ${} in phpdoc", tag_var.var_name));
      continue;
    }

    // for multiple @var inside op_phpdoc_raw we create an op_seq
    result = result ? (VertexPtr)VertexAdaptor<op_seq>::create(result, phpdoc_var) : phpdoc_var;
  }

  return result ?: (VertexPtr)VertexAdaptor<op_empty>::create();
}

VertexPtr ConvertLocalPhpdocsPass::on_enter_vertex(VertexPtr root) {
  if (auto phpdoc_raw = root.try_as<op_phpdoc_raw>()) {
    return visit_phpdoc_and_extract_vars(phpdoc_raw);
    // after this pipe the AST doesn't contain op_phpdoc_raw
    // (they are replaced with op_phpdoc_var)
  }

  return root;
}
