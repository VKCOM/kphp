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

static bool needs_lambda_prefix(FunctionPtr func) {
  return func->is_lambda() ||
         (func->modifiers.is_instance() && func->class_id->is_lambda_class()) ||
         (func->modifiers.is_instance() && func->class_id->is_typed_callable_interface());
}

static bool is_bundle_func(FunctionPtr func) {
  return !func->file_id->is_builtin() && !needs_lambda_prefix(func);
}

static int estimate_func_body_size(FunctionPtr func) {
  auto func_body = func->root->cmd();
  auto first_stmt_location = (*func_body->begin())->location;
  auto last_stmt_location = (*func_body->back()).location;
  if (last_stmt_location.line == -1 && func_body->size() >= 3) {
    last_stmt_location = func_body->args()[func_body->size()-1]->location;
  }
  if (last_stmt_location.line == -1 || first_stmt_location.line == -1) {
    // fall back to some arbitrary size approximation
    // we shouldn't even need it, but some locations are set to -1
    // and there is not much we can do about it here
    // (the compiler should try harder to avoid inserting the nodes without appropriate locations)
    return 10;
  }
  return last_stmt_location.line - first_stmt_location.line + 1;
}

void CodeGenF::execute(FunctionPtr function, DataStream<std::unique_ptr<CodeGenRootCmd>> &unused_os __attribute__ ((unused))) {
  if (function->does_need_codegen() || function->is_imported_from_static_lib()) {
    // prepare_generate_function() is used inside on_finish(), when we have all required functions available to us
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

  struct FuncBundle {
    SrcFilePtr file = {};
    std::vector<std::vector<FunctionPtr>> chunks = {{}};
    int current_chunk_size = 0;

    FuncBundle() = default;
    FuncBundle(SrcFilePtr file): file{file} {}

    void push_func(FunctionPtr f, int max_bundle_size) {
      int f_size = estimate_func_body_size(f);
      if (!f->is_inline && f_size > 10) {
        f_size /= 10;
      }
      std::vector<FunctionPtr> *chunk = &chunks.back();
      if (current_chunk_size != 0 && current_chunk_size + f_size > max_bundle_size) {
        current_chunk_size = 0;
        chunks.emplace_back();
        chunk = &chunks.back();
      }
      chunk->emplace_back(f);
      current_chunk_size += f_size;
    }

    static std::string add_chunk_suffix(int index, const std::string &name) {
      if (index == 0) {
        return name;
      }
      return name + "_" + std::to_string(index);
    }
  };

  // we want to group related functions into a single h/cpp files; these groups are called [func] bundles here
  // to build stable bundles, we need to have functions sorted on the across the bundle boundary (they're never bigger than a single file right now)
  //
  // linked list sort is nlogn; we can also use vectors instead of maps on the next step because of the file sorting included here
  all_functions.sort([](FunctionPtr a, FunctionPtr b) {
    return std::tie(a->file_id, a->name) < std::tie(b->file_id, b->name);
  });

  std::unordered_map<ClassPtr, FuncBundle> private_bundles;
  std::vector<FuncBundle> filename_bundles;
  std::vector<FunctionPtr> standalone_funcs;
  SrcFilePtr prev_file;

  for (FunctionPtr f : all_functions) {
    if (!is_bundle_func(f)) {
      // lambdas or something else that we don't want to group with other functions
      standalone_funcs.emplace_back(f);
      continue;
    }
    constexpr int FUNC_BUNDLE_MAX_SIZE = 400;
    // private methods are perfect buckets: they pack the things used together nicely
    if (f->modifiers.is_private()) {
      private_bundles[f->class_id].push_func(f, 2 * FUNC_BUNDLE_MAX_SIZE);
      continue;
    }
    if (prev_file != f->file_id) {
      prev_file = f->file_id;
      filename_bundles.emplace_back(f->file_id); // create next (empty) bundle
    }
    filename_bundles.back().push_func(f, FUNC_BUNDLE_MAX_SIZE);
  }

  // now run a prepare_generate_function for every function
  for (auto &f : standalone_funcs) {
    auto file_name = f->name;
    std::replace(file_name.begin(), file_name.end(), '$', '@');
    f->subdir = calc_subdir_for_function(f, f->file_id->short_file_name);
    prepare_generate_function(f, file_name);
  }
  for (auto &it : private_bundles) {
    auto file_name = it.first->src_name + "-private";
    std::replace(file_name.begin(), file_name.end(), '$', '@');
    for (int i = 0; i < it.second.chunks.size(); i++) {
      std::string chunk_file_name = FuncBundle::add_chunk_suffix(i, file_name);
      const auto &funcs = it.second.chunks[i];
      for (const auto &f : funcs) {
        f->subdir = calc_subdir_for_function(f, file_name); // using file_name because chunks belong to the same folder
        prepare_generate_function(f, chunk_file_name);
      }
    }
  }
  for (auto &it : filename_bundles) {
    auto file_name = static_cast<std::string>(G->calc_relative_name(it.file, false));
    std::replace(file_name.begin(), file_name.end(), '/', '@');
    for (int i = 0; i < it.chunks.size(); i++) {
      std::string chunk_file_name = FuncBundle::add_chunk_suffix(i, file_name);
      const auto &funcs = it.chunks[i];
      for (const auto &f : funcs) {
        f->subdir = calc_subdir_for_function(f, file_name); // using file_name because chunks belong to the same folder
        prepare_generate_function(f, chunk_file_name);
      }
    }
  }

  // spawn codegen tasks for functions
  for (const auto &f : standalone_funcs) {
    code_gen_start_root_task(os, std::make_unique<FunctionH>(f));
    code_gen_start_root_task(os, std::make_unique<FunctionCpp>(f));
  }
  for (const auto &it : private_bundles) {
    for (const auto &funcs : it.second.chunks) {
      if (funcs.empty()) {
        continue;
      }
      code_gen_start_root_task(os, std::make_unique<FunctionListH>(funcs));
      code_gen_start_root_task(os, std::make_unique<FunctionListCpp>(funcs));
    }
  }
  for (const auto &it : filename_bundles) {
    for (const auto &funcs : it.chunks) {
      if (funcs.empty()) {
        continue;
      }
      code_gen_start_root_task(os, std::make_unique<FunctionListH>(funcs));
      code_gen_start_root_task(os, std::make_unique<FunctionListCpp>(funcs));
    }
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

void CodeGenF::prepare_generate_function(FunctionPtr func, const std::string &file_name) {
  func->header_name = file_name + ".h";

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

std::string CodeGenF::calc_subdir_for_function(FunctionPtr func, const std::string &file_name) {
  // place __construct and __invoke of lambdas to a separate dir, like lambda classes are placed to cl_l/
  if (needs_lambda_prefix(func)) {
    return "o_l";
  }

  int bucket = vk::std_hash(file_name) % 100;
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
