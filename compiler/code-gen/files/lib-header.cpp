// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/lib-header.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/gen-out-style.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/compiler-core.h"
#include "compiler/data/lib-data.h"

void StaticLibraryRunGlobalHeaderH::compile(CodeGenerator& W) const {
  const std::string header_path = LibData::headers_tmp_dir() + LibData::run_global_function_name(G->get_global_namespace()) + ".h";

  W << OpenFile(header_path, "", false);

  W << "#pragma once" << NL << NL;
  W << StaticLibraryRunGlobal(gen_out_style::cpp) << ";" << NL;
  W << CloseFile();
}

LibHeaderTxt::LibHeaderTxt(std::vector<FunctionPtr>&& exported_functions)
    : exported_functions(std::move(exported_functions)) {}

void LibHeaderTxt::compile(CodeGenerator& W) const {
  W << OpenFile(LibData::functions_txt_tmp_file(), "", false, false);

  W << "<?php" << NL << NL;
  W << StaticLibraryRunGlobal(gen_out_style::txt) << ';' << NL << NL;

  for (const auto& function : exported_functions) {
    W << FunctionDeclaration(function, true, gen_out_style::txt) << ";" << NL;
  }

  W << CloseFile();
}

LibHeaderH::LibHeaderH(FunctionPtr exported_function)
    : exported_function(exported_function) {}

void LibHeaderH::compile(CodeGenerator& W) const {
  W << OpenFile(LibData::headers_tmp_dir() + exported_function->header_name, "", false);

  W << "#pragma once" << NL;
  W << ExternInclude(G->settings().runtime_headers.get()) << NL;

  W << OpenNamespace() << FunctionDeclaration(exported_function, true, gen_out_style::cpp) << ";" << NL << CloseNamespace();

  W << "using " << G->get_global_namespace() << "::" << FunctionName(exported_function) << ";" << NL;
  W << CloseFile();
}
