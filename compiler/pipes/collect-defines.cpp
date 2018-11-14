#include "compiler/pipes/collect-defines.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/define-data.h"

VertexPtr CollectDefinesPass::on_exit_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_define) {
    VertexAdaptor<op_define> define = root;
    VertexPtr name = define->name();
    VertexPtr val = define->value();

    kphp_error_act (
      name->type() == op_string,
      "Define: first parameter must be a string",
      return root
    );

    DefineData::DefineType def_type;
    if (!CheckConst::is_const(val)) {
      kphp_error(name->get_string().length() <= 1 || name->get_string().substr(0, 2) != "c#",
                 dl_pstr("Couldn't calculate value of %s", name->get_string().substr(2).c_str()));
      def_type = DefineData::def_var;

      auto var = VertexAdaptor<op_var>::create();
      var->extra_type = op_ex_var_superglobal;
      var->str_val = name->get_string();
      set_location(var, name->get_location());
      name = var;

      define->value() = VertexPtr();
      auto new_root = VertexAdaptor<op_set>::create(name, val);
      set_location(new_root, root->get_location());
      root = new_root;
    } else {
      def_type = DefineData::def_php;
      auto new_root = VertexAdaptor<op_empty>::create();
      root = new_root;
    }

    DefineData *data = new DefineData(val, def_type);
    data->name = name->get_string();
    data->file_id = stage::get_file();
    DefinePtr def_id(data);

    if (def_type == DefineData::def_var) {
      name->set_string("d$" + name->get_string());
    } else {
      current_function->define_ids.push_back(def_id);
    }

    G->register_define(def_id);
  }

  return root;
}
