// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/code-gen.h"

#include "compiler/code-gen/code-gen-task.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/const-globals-batched-mem.h"
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
#include "compiler/data/generics-mixins.h"
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
  const GlobalsBatchedMem &all_globals_in_mem = GlobalsBatchedMem::prepare_mem_and_assign_offsets(all_globals);

  std::vector<VarPtr> all_constants = G->get_constants_vars();
  const ConstantsBatchedMem &all_constants_in_mem = ConstantsBatchedMem::prepare_mem_and_assign_offsets(all_constants);

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

  if (G->settings().enable_global_vars_memory_stats.get() && !G->is_output_mode_lib() && !G->is_output_mode_k2_lib()) {
    code_gen_start_root_task(os, std::make_unique<GlobalVarsMemoryStats>(all_globals));
  }
  code_gen_start_root_task(os, std::make_unique<InitScriptsCpp>(G->get_main_file()));
  code_gen_start_root_task(os, std::make_unique<ConstVarsInit>(all_constants_in_mem));
  code_gen_start_root_task(os, std::make_unique<GlobalVarsReset>(all_globals_in_mem));

  if (G->is_output_mode_lib() || G->is_output_mode_k2_lib()) {
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
  if (!G->is_output_mode_lib() && !G->is_output_mode_k2_lib()) {
    if (!G->is_output_mode_k2()) {
      code_gen_start_root_task(os, std::make_unique<TypeTagger>(vk::singleton<ForkableTypeStorage>::get().flush_forkable_types(),
                                                                vk::singleton<ForkableTypeStorage>::get().flush_waitable_types()));
    }
    code_gen_start_root_task(os, std::make_unique<ShapeKeys>(TypeHintShape::get_all_registered_keys()));
    code_gen_start_root_task(os, std::make_unique<JsonEncoderTags>(std::move(all_json_encoders)));
    code_gen_start_root_task(os, std::make_unique<CmakeListsTxt>());
  }

  if (!TracingAutogen::empty()) {
    code_gen_start_root_task(os, std::make_unique<TracingAutogen>());
  }

  code_gen_start_root_task(os, std::make_unique<TlSchemaToCpp>());
  code_gen_start_root_task(os, std::make_unique<LibVersionHFile>());
  if (!G->is_output_mode_lib() && !G->is_output_mode_k2()) {
    code_gen_start_root_task(os, std::make_unique<CppMainFile>());
  }
  if (G->is_output_mode_k2()) {
    code_gen_start_root_task(os, std::make_unique<ComponentInfoFile>());
  }
}

void CodeGenF::prepare_generate_function(FunctionPtr func) {
  std::string file_name = func->name;

  // shorten very long file names: they were generated by long namespaces (full class names)
  // provided solution is to replace "very\\long\\namespace\\classname" with "{namespace_hash}\\classname"
  // note, that we do this only at the moment of codegen (not at the moment of function name generation),
  // because original (long) function names are sometimes parsed to show proper compilation errors
  // (for instance, from "baseclass$$method$$contextclass" static=contextclass is parsed)
  if (file_name.size() > 100 && func->is_instantiation_of_generic_function()) {
    for (const auto &[_, type_hint] : *func->instantiationTs) {
      type_hint->traverse([&file_name, this](const TypeHint *child) {
        if (const auto *as_instance = child->try_as<TypeHintInstance>()) {
          file_name = shorten_occurence_of_class_in_file_name(as_instance->resolve(), file_name);
        }
      });
    }
  }
  if (file_name.size() > 100 && func->context_class && func->class_id != func->context_class) {
    file_name = shorten_occurence_of_class_in_file_name(func->context_class, file_name);
  }
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

std::string CodeGenF::shorten_occurence_of_class_in_file_name(ClassPtr occuring_class, const std::string &file_name) {
  size_t pos = occuring_class->name.rfind('\\');
  if (pos == std::string::npos) {
    return file_name;
  }
  vk::string_view occuring_namespace = vk::string_view{occuring_class->name}.substr(0, pos);
  vk::string_view occuring_local_name = vk::string_view{occuring_class->name}.substr(pos + 1);

  std::string occurence1 = replace_characters(occuring_class->name, '\\', '$');
  std::string occurence2 = replace_characters(occuring_class->name, '\\', '_');
  std::string short_occur = fmt_format("{:x}${}", static_cast<uint32_t>(vk::std_hash(occuring_namespace)), occuring_local_name);

  std::string shortened = file_name;
  shortened = vk::replace_all(shortened, occurence1, short_occur);
  shortened = vk::replace_all(shortened, occurence2, short_occur);
  // printf("shortened file_name:\n    %s\n -> %s\n", file_name.c_str(), shortened.c_str());
  return shortened;
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

  int bucket = vk::std_hash(func->file_id->short_file_name) % G->settings().file_buckets_count.get();
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
