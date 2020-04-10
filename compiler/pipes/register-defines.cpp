#include "compiler/pipes/register-defines.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"

VertexPtr RegisterDefinesPass::on_exit_vertex(VertexPtr root, LocalT *) {
  // дефайны — во-первых, это явное define('name',value)
  if (auto define = root.try_as<op_define>()) {
    VertexPtr name = define->name();
    VertexPtr val = define->value();

    kphp_error_act(name->type() == op_string, "Define name should be a valid string", return root);

    auto data = new DefineData(name->get_string(), val, DefineData::def_unknown);
    data->file_id = stage::get_file();
    G->register_define(DefinePtr(data));

    // на данный момент define() мы оставляем; если он окажется константой в итоге,
    // то удалится в EraseDefinesDeclarationsPass
  }

  // во-вторых, это константы класса — они не добавляются никуда в ast дерево, хранятся отдельно
  if (root->type() == op_function && current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->register_defines();
  }

  return root;
}
