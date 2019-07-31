#pragma once

#include <string>

#include "common/tlo-parsing/tlo-parsing-tools.h"

#include "compiler/data/data_ptr.h"

class CodeGenerator;

namespace tl_gen {

void write_tl_query_handlers(CodeGenerator &W);

std::string cpp_tl_struct_name(const char *prefix, std::string tl_name, const std::string &template_args_postfix = "");

ClassPtr get_php_class_of_tl_function(vk::tl::combinator *f);
std::string get_tl_function_of_php_class(ClassPtr klass);

bool is_php_class_a_tl_function(ClassPtr klass);
bool is_php_class_a_tl_constructor(ClassPtr klass);
bool is_php_class_a_tl_polymorphic_type(ClassPtr klass);

} // namespace tl_gen
