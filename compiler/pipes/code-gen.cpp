#include "compiler/pipes/code-gen.h"

#include "compiler/code-gen/code-gen-task.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/files/function-header.h"
#include "compiler/code-gen/files/function-source.h"
#include "compiler/code-gen/files/init-scripts.h"
#include "compiler/code-gen/files/lib-header.h"
#include "compiler/code-gen/files/tl2cpp.h"
#include "compiler/code-gen/files/type-tagger.h"
#include "compiler/code-gen/files/vars-cpp.h"
#include "compiler/code-gen/files/vars-reset.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/inferring/public.h"

class CollectForkableTypes : public FunctionPassBase {
public:
  std::vector<const TypeData *> waitable_types;
  std::vector<const TypeData *> forkable_types;

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (auto call = root.try_as<op_func_call>()) {
      if (call->func_id->is_resumable) {
        if (call->str_val == "wait_result") {
          waitable_types.push_back(tinf::get_type(root));
        } else if (call->str_val == "wait_result_multi") {
          forkable_types.push_back(tinf::get_type(root)->const_read_at(Key::any_key()));
          waitable_types.push_back(tinf::get_type(root)->const_read_at(Key::any_key()));
        }
        forkable_types.push_back(tinf::get_type(root));
      }
    }
    return root;
  }
};

void CodeGenF::execute(FunctionPtr function, DataStream<WriterData> &os) {
  CollectForkableTypes pass;
  run_function_pass(function, &pass);
  if (!pass.waitable_types.empty() || !pass.forkable_types.empty()){
    static volatile int lock = 0;
    AutoLocker<volatile int*> locker(&lock);
    waitable_types.insert(waitable_types.end(), pass.waitable_types.begin(), pass.waitable_types.end());
    forkable_types.insert(forkable_types.end(), pass.forkable_types.begin(), pass.forkable_types.end());
  }

  std::call_once(dest_dir_synced, [] {
    G->init_dest_dir();
    G->load_index();
  });
  SyncPipeF<FunctionPtr, WriterData>::execute(function, os);
}

size_t CodeGenF::calc_count_of_parts(size_t cnt_global_vars) {
  return 1u + cnt_global_vars / 4096u;
}

void CodeGenF::on_finish(DataStream<WriterData> &os) {
  stage::set_name("GenerateCode");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  auto xall = tmp_stream.flush();
  xall.sort();
  const vector<ClassPtr> &all_classes = G->get_classes();

  //TODO: delete W_ptr
  CodeGenerator *W_ptr = new CodeGenerator(os);
  CodeGenerator &W = *W_ptr;

  vector<SrcFilePtr> main_files = G->get_main_files();
  std::unordered_set<FunctionPtr> main_functions(main_files.size());
  for (const SrcFilePtr &main_file : main_files) {
    main_functions.emplace(main_file->main_function);
  }

  auto should_gen_function = [&](FunctionPtr fun) {
    return fun->type != FunctionData::func_class_holder &&
           (
             fun->body_seq != FunctionData::body_value::empty ||
             main_functions.count(fun)
           );
  };

  //TODO: parallelize;
  for (const auto &fun : xall) {
    if (should_gen_function(fun)) {
      prepare_generate_function(fun);
    }
  }
  for (const auto &c : all_classes) {
    if (ClassData::does_need_codegen(c)) {
      prepare_generate_class(c);
    }
  }

  vector<FunctionPtr> all_functions;
  vector<FunctionPtr> exported_functions;

  for (const auto &function : xall) {
    if (!should_gen_function(function)) {
      continue;
    }
    if (function->is_extern()) {
      continue;
    }
    all_functions.push_back(function);
    W << Async(FunctionH(function));
    W << Async(FunctionCpp(function));

    if (function->kphp_lib_export && G->env().is_static_lib_mode()) {
      exported_functions.emplace_back(function);
    }
  }

  for (const auto &c : all_classes) {
    if (!ClassData::does_need_codegen(c)) {
      continue;
    }

    switch (c->class_type) {
      case ClassType::klass:
        W << Async(ClassDeclaration(c));
        break;
      case ClassType::interface:
        W << Async(InterfaceDeclaration(c));
        break;

      case ClassType::trait:
        /* fallthrough */
      default:
        kphp_assert(false);
    }
  }

  for (const auto &main_file : main_files) {
    W << Async(GlobalVarsReset(main_file));
  }
  W << Async(InitScriptsCpp(std::move(main_files), std::move(all_functions)));

  std::vector<VarPtr> vars = G->get_global_vars();
  for (FunctionPtr fun: xall) {
    vars.insert(vars.end(), fun->static_var_ids.begin(), fun->static_var_ids.end());
  }
  size_t parts_cnt = calc_count_of_parts(vars.size());
  W << Async(VarsCpp(std::move(vars), parts_cnt));

  if (G->env().is_static_lib_mode()) {
    for (FunctionPtr exported_function: exported_functions) {
      W << Async(LibHeaderH(exported_function));
    }
    W << Async(LibHeaderTxt(std::move(exported_functions)));
    W << Async(StaticLibraryRunGlobalHeaderH());
  } else {
    // TODO: should be done in lib mode, but by some other way
    W << Async(TypeTagger(std::move(forkable_types), std::move(waitable_types)));
  }

  write_tl_schema(W);
  //TODO: use Async for that
  tl_gen::write_tl_query_handlers(W);
  write_lib_version(W);
}

void CodeGenF::prepare_generate_function(FunctionPtr func) {
  string file_name = func->name;
  std::replace(file_name.begin(), file_name.end(), '$', '@');

  func->header_name = file_name + ".h";
  func->subdir = get_subdir(func->file_id->short_file_name);

  if (!func->is_inline) {
    func->src_name = file_name + ".cpp";
  }

  func->header_full_name =
    func->is_imported_from_static_lib()
    ? func->file_id->owner_lib->headers_dir() + func->header_name
    : func->subdir + "/" + func->header_name;

  my_unique(&func->static_var_ids);
  my_unique(&func->global_var_ids);
  my_unique(&func->local_var_ids);
}

string CodeGenF::get_subdir(const string &base) {
  int bucket = vk::std_hash(base) % 100;
  return "o_" + std::to_string(bucket);
}

void CodeGenF::write_tl_schema(CodeGenerator &W) {
  string schema;
  int schema_length = -1;
  if (!G->env().get_tl_schema_file().empty()) {
    FILE *f = fopen(G->env().get_tl_schema_file().c_str(), "r");
    const int MAX_SCHEMA_LEN = 1024 * 1024;
    static char buf[MAX_SCHEMA_LEN + 1];
    kphp_assert (f && "can't open tl schema file");
    schema_length = fread(buf, 1, sizeof(buf), f);
    kphp_assert (schema_length > 0 && schema_length < MAX_SCHEMA_LEN);
    schema.assign(buf, buf + schema_length);
    kphp_assert (!fclose(f));
  }
  if (!G->env().is_static_lib_mode()) {
    W << OpenFile("_tl_schema.cpp", "", false);
    W << "const char *builtin_tl_schema = " << NL << Indent(2);
    compile_string_raw(schema, W);
    W << ";" << NL;
    W << "int builtin_tl_schema_length = " << schema_length << ";" << NL;
    W << CloseFile();
  }
}

void CodeGenF::write_lib_version(CodeGenerator &W) {
  W << OpenFile("_lib_version.h");
  W << "//" << G->env().get_runtime_sha256() << NL;
  W << CloseFile();
}

void CodeGenF::prepare_generate_class(ClassPtr) {
}
