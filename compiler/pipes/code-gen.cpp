// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/code-gen.h"

#include "compiler/cpp-dest-dir-initializer.h"
#include "compiler/code-gen/code-gen-task.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/files/cmake-lists-txt.h"
#include "compiler/code-gen/files/function-header.h"
#include "compiler/code-gen/files/function-source.h"
#include "compiler/code-gen/files/json-encoder-tags.h"
#include "compiler/code-gen/files/global_vars_memory_stats.h"
#include "compiler/code-gen/files/init-scripts.h"
#include "compiler/code-gen/files/lib-header.h"
#include "compiler/code-gen/files/tl2cpp/tl2cpp.h"
#include "compiler/code-gen/files/shape-keys.h"
#include "compiler/code-gen/files/tracing-autogen.h"
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
#include "compiler/pipes/collect-forkable-types.h"
#include "compiler/type-hint.h"

size_t CodeGenF::calc_count_of_parts(size_t cnt_global_vars) {
  return 1u + cnt_global_vars / G->settings().globals_split_count.get();
}


void CodeGenF::execute(FunctionPtr function, DataStream<std::unique_ptr<CodeGenRootCmd>> &unused_os __attribute__ ((unused))) {
  if (function->does_need_codegen() || function->is_imported_from_static_lib()) {
    prepare_generate_function(function);
    G->stats.on_function_processed(function);
    tmp_stream << function;
  }
}

void CodeGenF::on_finish(DataStream<std::unique_ptr<CodeGenRootCmd>> &os) {
  vk::singleton<CppDestDirInitializer>::get().wait();

  G->get_ffi_root().bind_symbols();
  TracingAutogen::finished_appending_and_prepare();

  stage::set_name("GenerateCode");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  std::forward_list<FunctionPtr> all_functions = tmp_stream.flush();   // functions to codegen, order doesn't matter
  const std::vector<ClassPtr> &all_classes = G->get_classes();
  std::set<ClassPtr> all_json_encoders;

  for (FunctionPtr f : all_functions) {
    code_gen_start_root_task(os, std::make_unique<FunctionH>(f));
    code_gen_start_root_task(os, std::make_unique<FunctionCpp>(f));
  }

  for (ClassPtr c : all_classes) {
    if (c->kphp_json_tags && G->get_class("JsonEncoder")->is_parent_of(c)) {
      all_json_encoders.insert(c);
    }
    if (!c->does_need_codegen()) {
      continue;
    }

    switch (c->class_type) {
      case ClassType::klass:
        code_gen_start_root_task(os, std::make_unique<ClassDeclaration>(c));
        code_gen_start_root_task(os, std::make_unique<ClassMembersDefinition>(c));
        break;
      case ClassType::interface:
        code_gen_start_root_task(os, std::make_unique<InterfaceDeclaration>(c));
        break;
      case ClassType::ffi_scope:
        code_gen_start_root_task(os, std::make_unique<FFIDeclaration>(c));
        break;

      default:
        kphp_assert(false);
    }
  }

  code_gen_start_root_task(os, std::make_unique<GlobalVarsReset>(G->get_main_file()));
  if (G->settings().enable_global_vars_memory_stats.get()) {
    code_gen_start_root_task(os, std::make_unique<GlobalVarsMemoryStats>(G->get_main_file()));
  }
  code_gen_start_root_task(os, std::make_unique<InitScriptsCpp>(G->get_main_file()));

  std::vector<VarPtr> vars = G->get_global_vars();
  for (FunctionPtr f : all_functions) {
    vars.insert(vars.end(), f->static_var_ids.begin(), f->static_var_ids.end());
  }
  size_t parts_cnt = calc_count_of_parts(vars.size());

  std::vector<std::vector<VarPtr>> vars_batches(parts_cnt);
  std::vector<int> max_dep_levels(parts_cnt);
  for (VarPtr var : vars) {
    vars_batches[vk::std_hash(var->name) % parts_cnt].emplace_back(var);
  }
  for (size_t part_id = 0; part_id < parts_cnt; ++part_id) {
    int max_dep_level{0};
    for (auto var : vars_batches[part_id]) {
      if (var->is_constant() && max_dep_level < var->dependency_level) {
        max_dep_level = var->dependency_level;
      }
    }
    max_dep_levels[part_id] = max_dep_level;
    code_gen_start_root_task(os, std::make_unique<VarsCppPart>(std::move(vars_batches[part_id]), part_id));
  }
  code_gen_start_root_task(os, std::make_unique<VarsCpp>(std::move(max_dep_levels), parts_cnt));

  if (G->settings().is_static_lib_mode()) {
    std::vector<FunctionPtr> exported_functions;
    for (FunctionPtr f : all_functions) {
      if (f->kphp_lib_export) {
        code_gen_start_root_task(os, std::make_unique<LibHeaderH>(f));
        exported_functions.emplace_back(f);
      }
    }
    std::sort(exported_functions.begin(), exported_functions.end());
    code_gen_start_root_task(os, std::make_unique<LibHeaderTxt>(std::move(exported_functions)));
    code_gen_start_root_task(os, std::make_unique<StaticLibraryRunGlobalHeaderH>());
  }

  // TODO: should be done in lib mode also, but in some other way
  if (!G->settings().is_static_lib_mode()) {
    code_gen_start_root_task(os, std::make_unique<TypeTagger>(vk::singleton<ForkableTypeStorage>::get().flush_forkable_types(), vk::singleton<ForkableTypeStorage>::get().flush_waitable_types()));
    code_gen_start_root_task(os, std::make_unique<ShapeKeys>(TypeHintShape::get_all_registered_keys()));
    code_gen_start_root_task(os, std::make_unique<JsonEncoderTags>(std::move(all_json_encoders)));
    code_gen_start_root_task(os, std::make_unique<CmakeListsTxt>());
  }

  if (!TracingAutogen::empty()) {
    code_gen_start_root_task(os, std::make_unique<TracingAutogen>());
  }

  code_gen_start_root_task(os, std::make_unique<TlSchemaToCpp>());
  code_gen_start_root_task(os, std::make_unique<LibVersionHFile>());
  if (!G->settings().is_static_lib_mode()) {
    code_gen_start_root_task(os, std::make_unique<CppMainFile>());
  }
}

void CodeGenF::prepare_generate_function(FunctionPtr func) {
  std::string file_name = func->name;
  std::replace(file_name.begin(), file_name.end(), '$', '@');

  func->header_name = file_name + ".h";
  func->subdir = calc_subdir_for_function(func);

  if (!func->is_inline) {
    func->src_name = file_name + ".cpp";
  }

  func->header_full_name =
    func->is_imported_from_static_lib()
    ? func->file_id->owner_lib->headers_dir() + func->header_name
    : func->subdir + "/" + func->header_name;

  std::sort(func->static_var_ids.begin(), func->static_var_ids.end());
  std::sort(func->global_var_ids.begin(), func->global_var_ids.end());
  std::sort(func->local_var_ids.begin(), func->local_var_ids.end());

  if (func->kphp_tracing) {
    TracingAutogen::register_function_marked_kphp_tracing(func);
  }
}

std::string CodeGenF::calc_subdir_for_function(FunctionPtr func) {
  // place __construct and __invoke of lambdas to a separate dir, like lambda classes are placed to cl_l/
  if (func->is_lambda()) {
    return "o_l";
  }
  if (func->modifiers.is_instance() && func->class_id->is_lambda_class()) {
    return "o_l";
  }
  if (func->modifiers.is_instance() && func->class_id->is_typed_callable_interface()) {
    return "o_l";
  }

  int bucket = vk::std_hash(func->file_id->short_file_name) % 100;
  return "o_" + std::to_string(bucket);
}


void CodeGenForDiffF::execute(std::unique_ptr<CodeGenRootCmd> cmd, DataStream<WriterData *> &os) {
  stage::set_name("Code generation for diff");

  // re-launch cmd not with "calc hashes", but with "store cpp/h contents and php comments" mode
  // all generated files will be passed to os (to WriteFilesF)
  CodeGenerator W(false, os);
  cmd->compile(W);
  // now cmd is destroyed
}
