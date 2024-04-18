// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/code-gen.h"

#include "compiler/code-gen/code-gen-task.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-linear-mem.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/files/cmake-lists-txt.h"
#include "compiler/code-gen/files/const-vars-init.h"
#include "compiler/code-gen/files/function-header.h"
#include "compiler/code-gen/files/function-source.h"
#include "compiler/code-gen/files/global-vars-memory-stats.h"
#include "compiler/code-gen/files/global-vars-reset.h"
#include "compiler/code-gen/files/init-scripts.h"
#include "compiler/code-gen/files/json-encoder-tags.h"
#include "compiler/code-gen/files/lib-header.h"
#include "compiler/code-gen/files/shape-keys.h"
#include "compiler/code-gen/files/tl2cpp/tl2cpp.h"
#include "compiler/code-gen/files/tracing-autogen.h"
#include "compiler/code-gen/files/type-tagger.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/compiler-core.h"
#include "compiler/cpp-dest-dir-initializer.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"
#include "compiler/pipes/collect-forkable-types.h"
#include "compiler/type-hint.h"

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

  std::vector<VarPtr> all_globals = G->get_global_vars();
  for (FunctionPtr f : all_functions) {
    all_globals.insert(all_globals.end(), f->static_var_ids.begin(), f->static_var_ids.end());
  }
  prepare_generate_globals(all_globals);

  std::vector<VarPtr> all_constants = G->get_constants_vars();
  prepare_generate_constants(all_constants);

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

  if (G->settings().enable_global_vars_memory_stats.get() && !G->is_output_mode_lib()) {
    code_gen_start_root_task(os, std::make_unique<GlobalVarsMemoryStats>(all_globals));
  }
  code_gen_start_root_task(os, std::make_unique<InitScriptsCpp>(G->get_main_file()));

  // todo rethink globals_split_count after chunks in memory
  code_gen_start_root_task(os, std::make_unique<ConstVarsInit>(std::move(all_constants), G->settings().globals_split_count.get()));
  code_gen_start_root_task(os, std::make_unique<GlobalVarsReset>(std::move(all_globals), G->settings().globals_split_count.get()));

  if (G->is_output_mode_lib()) {
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
  if (!G->is_output_mode_lib()) {
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
  if (!G->is_output_mode_lib()) {
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

  std::sort(func->local_var_ids.begin(), func->local_var_ids.end(), [](VarPtr v1, VarPtr v2) {
    return v1->name.compare(v2->name) < 0;
  });

  if (func->kphp_tracing) {
    TracingAutogen::register_function_marked_kphp_tracing(func);
  }
}

void CodeGenF::prepare_generate_globals(std::vector<VarPtr> &all_globals) {
  // sort variables by name to make codegen stable
  // note, that all_globals contains also function static vars (explicitly added),
  // and their names can duplicate or be equal to global vars;
  // hence, also sort by holder_func (though global vars don't have holder_func, since there's no point of declaration)
  std::sort(all_globals.begin(), all_globals.end(), [](VarPtr c1, VarPtr c2) -> bool {
    int cmp_name = c1->name.compare(c2->name);
    if (cmp_name < 0) {
      return true;
    } else if (cmp_name > 0) {
      return false;
    } else if (c1 == c2) {
      return false;
    } else {
      if (!c1->holder_func) return true;
      if (!c2->holder_func) return false;
      return c1->holder_func->name.compare(c2->holder_func->name) < 0;
    }
  });

  GlobalsLinearMem::prepare_mem_and_assign_offsets(all_globals);
}

void CodeGenF::prepare_generate_constants(std::vector<VarPtr> &all_constants) {
  // sort constants by name to make codegen stable
  std::sort(all_constants.begin(), all_constants.end(), [](VarPtr c1, VarPtr c2) -> bool {
    return c1->name.compare(c2->name) < 0;
  });

  ConstantsLinearMem::prepare_mem_and_assign_offsets(all_constants);
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
