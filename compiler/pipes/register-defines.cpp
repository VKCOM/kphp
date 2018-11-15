#include "compiler/pipes/register-defines.h"

#include "compiler/compiler-core.h"

VertexPtr RegisterDefinesPass::on_exit_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_define) {
    VertexAdaptor<op_define> define = root;
    VertexPtr name = define->name();
    VertexPtr val = define->value();

    kphp_error_act(name->type() == op_string,
                   dl_pstr("Define name should be a valid string"),
                   return root);

    DefineData *data = new DefineData(val, DefineData::def_unknown);
    data->name = name->get_string();
    data->file_id = stage::get_file();
    G->register_define(DefinePtr(data));

    // на данный момент define() мы оставляем; если он окажется константой в итоге,
    // то удалится в EraseDefinesDeclarationsPass
  }
  return root;
}
