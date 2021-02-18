// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/code-gen.h"

#include "compiler/cpp-dest-dir-initializer.h"
#include "compiler/code-gen/code-gen-task.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/files/function-header.h"
#include "compiler/code-gen/files/function-source.h"
#include "compiler/code-gen/files/global_vars_memory_stats.h"
#include "compiler/code-gen/files/init-scripts.h"
#include "compiler/code-gen/files/lib-header.h"
#include "compiler/code-gen/files/tl2cpp/tl2cpp.h"
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

size_t CodeGenF::calc_count_of_parts(size_t cnt_global_vars) {
  return 1u + cnt_global_vars / G->settings().globals_split_count.get();
}


void CodeGenF::execute(FunctionPtr function, DataStream<WriterData> &unused_os __attribute__ ((unused))) {
  if (FunctionData::does_need_codegen(function)) {
    prepare_generate_function(function);
    G->stats.on_function_processed(function);
    tmp_stream << function;
  }
}

void CodeGenF::on_finish(DataStream<WriterData> &os) {
  vk::singleton<CppDestDirInitializer>::get().wait();

  stage::set_name("GenerateCode");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  std::forward_list<FunctionPtr> all_functions = tmp_stream.flush();   // functions to codegen, order doesn't matter
  const std::vector<ClassPtr> &all_classes = G->get_classes();

  //TODO: delete W_ptr
  CodeGenerator *W_ptr = new CodeGenerator(os);
  CodeGenerator &W = *W_ptr;  // created once, others are copied in Async()

  for (FunctionPtr f : all_functions) {
    W << Async(FunctionH(f));
    W << Async(FunctionCpp(f));
  }

  for (ClassPtr c : all_classes) {
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

      default:
        kphp_assert(false);
    }
  }

  W << Async(GlobalVarsReset(G->get_main_file()));
  if (G->settings().enable_global_vars_memory_stats.get()) {
    W << Async(GlobalVarsMemoryStats{G->get_main_file()});
  }
  W << Async(InitScriptsCpp(G->get_main_file()));

  std::vector<VarPtr> vars = G->get_global_vars();
  for (FunctionPtr f : all_functions) {
    vars.insert(vars.end(), f->static_var_ids.begin(), f->static_var_ids.end());
  }
  size_t parts_cnt = calc_count_of_parts(vars.size());
  W << Async(VarsCpp(std::move(vars), parts_cnt));

  if (G->settings().is_static_lib_mode()) {
    std::vector<FunctionPtr> exported_functions;
    for (FunctionPtr f : all_functions) {
      if (f->kphp_lib_export) {
        W << Async(LibHeaderH(f));
        exported_functions.emplace_back(f);
      }
    }
    std::sort(exported_functions.begin(), exported_functions.end());
    W << Async(LibHeaderTxt(std::move(exported_functions)));
    W << Async(StaticLibraryRunGlobalHeaderH());
  }

  // TODO: should be done in lib mode also, but in some other way
  if (!G->settings().is_static_lib_mode()) {
    W << Async(TypeTagger(vk::singleton<ForkableTypeStorage>::get().flush_forkable_types(), vk::singleton<ForkableTypeStorage>::get().flush_waitable_types()));
  }

  W << Async(TlSchemaToCpp());
  W << Async(LibVersionHFile());
  if (!G->settings().is_static_lib_mode()) {
    W << Async(CppMainFile());
  }
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

  std::sort(func->static_var_ids.begin(), func->static_var_ids.end());
  std::sort(func->global_var_ids.begin(), func->global_var_ids.end());
  std::sort(func->local_var_ids.begin(), func->local_var_ids.end());
}

string CodeGenF::get_subdir(const string &base) {
  int bucket = vk::std_hash(base) % 100;
  return "o_" + std::to_string(bucket);
}
