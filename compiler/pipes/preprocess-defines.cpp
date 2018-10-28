#include "compiler/pipes/preprocess-defines.h"

#include "common/version-string.h"

#include "compiler/data/define-data.h"

class CollectDefinesToVectorPass : public FunctionPassBase {
private:
  vector<VertexPtr> defines;
public:
  string get_description() {
    return "Process defines concatenation";
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_define) {
      defines.push_back(root);
    }
    return root;
  }

  const vector<VertexPtr> &get_vector() {
    return defines;
  }
};

PreprocessDefinesF::PreprocessDefinesF() :
  check_const(define_vertex),
  make_const(define_vertex) {
  defines_stream.set_sink(true);
  all_fun.set_sink(true);

  auto val = VertexAdaptor<op_string>::create();
  val->set_string(get_version_string());
  DefineData *data = new DefineData(val, DefineData::def_php);
  data->name = "KPHP_COMPILER_VERSION";
  DefinePtr def_id(data);

  G->register_define(def_id);
}
void PreprocessDefinesF::execute(FunctionPtr function, DataStream<FunctionPtr> &) {
  AUTO_PROF(preprocess_defines);
  CollectDefinesToVectorPass pass;
  run_function_pass(function, &pass);

  vector<VertexPtr> vs = pass.get_vector();
  for (size_t i = 0; i < vs.size(); i++) {
    defines_stream << vs[i];
  }
  all_fun << function;
}
void PreprocessDefinesF::on_finish(DataStream<FunctionPtr> &os) {
  AUTO_PROF(preprocess_defines_finish);
  stage::set_name("Preprocess defines");
  stage::set_file(SrcFilePtr());

  stage::die_if_global_errors();

  vector<VertexPtr> all_defines = defines_stream.get_as_vector();
  for (auto define : all_defines) {
    VertexPtr name_v = define.as<op_define>()->name();
    stage::set_location(define.as<op_define>()->location);
    kphp_error_return(
      name_v->type() == op_string,
      "Define: first parameter must be a string"
    );

    string name = name_v.as<op_string>()->str_val;
    if (!define_vertex.insert(make_pair(name, define)).second) {
      kphp_error_return(0, dl_pstr("Duplicate define declaration: %s", name.c_str()));
    }
  }

  for (auto define : all_defines) {
    process_define(define);
  }

  for (auto fun : all_fun.get_as_vector()) {
    os << fun;
  }
}
void PreprocessDefinesF::process_define_recursive(VertexPtr root) {
  if (root->type() == op_func_name) {
    string name = root.as<op_func_name>()->str_val;
    name = resolve_define_name(name);
    map<string, VertexPtr>::iterator it = define_vertex.find(name);
    if (it != define_vertex.end()) {
      process_define(it->second);
    }
  }
  for (auto i : *root) {
    process_define_recursive(i);
  }
}
void PreprocessDefinesF::process_define(VertexPtr root) {
  stage::set_location(root->location);
  VertexAdaptor<meta_op_define> define = root;
  VertexPtr name_v = define->name();
  VertexPtr val = define->value();

  kphp_error_return(
    name_v->type() == op_string,
    "Define: first parameter must be a string"
  );

  string name = name_v.as<op_string>()->str_val;

  if (done.find(name) != done.end()) {
    return;
  }

  if (in_progress.find(name) != in_progress.end()) {
    stringstream stream;
    int id = -1;
    for (size_t i = 0; i < stack.size(); i++) {
      if (stack[i] == name) {
        id = i;
        break;
      }
    }
    kphp_assert(id != -1);
    for (size_t i = id; i < stack.size(); i++) {
      stream << stack[i] << " -> ";
    }
    stream << name;
    kphp_error_return(0, dl_pstr("Recursive define dependency:\n%s\n", stream.str().c_str()));
  }

  in_progress.insert(name);
  stack.push_back(name);

  process_define_recursive(val);
  stage::set_location(root->location);

  in_progress.erase(name);
  stack.pop_back();
  done.insert(name);

  if (check_const.is_const(val)) {
    define->value() = make_const.make_const(val);
    kphp_assert(define->value());
  }
}
