#pragma once

#include "common/tlo-parsing/tl-objects.h"

#include "compiler/code-gen/common.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/data_ptr.h"
#include "compiler/utils/string-utils.h"

class CodeGenerator;

namespace tl2cpp {
extern const std::unordered_set<std::string> CUSTOM_IMPL_TYPES;    // from tl_builtins.h
extern vk::tl::tl_scheme *tl;
extern const vk::tl::combinator *cur_combinator;
extern const std::string T_TYPE;
extern std::set<std::string> tl_const_vars;

enum class field_rw_type {
  READ,
  WRITE
};

std::string get_tl_object_field_access(const std::unique_ptr<vk::tl::arg> &arg, field_rw_type rw_type);

vk::tl::type *type_of(const std::unique_ptr<vk::tl::type_expr_base> &type_expr, const vk::tl::tl_scheme *scheme = tl);
vk::tl::type *type_of(const vk::tl::type_expr *type, const vk::tl::tl_scheme *scheme = tl);

bool is_magic_processing_needed(const vk::tl::type_expr *type_expr);
std::string get_magic_storing(const vk::tl::type_expr_base *arg_type_expr);
std::string get_magic_fetching(const vk::tl::type_expr_base *arg_type_expr, const std::string &error_msg);
std::string cpp_tl_struct_name(const char *prefix, std::string tl_name, const std::string &template_args_postfix = "");

std::string cpp_tl_const_str(std::string tl_name);
std::string register_tl_const_str(const std::string &tl_name);
int64_t hash_tl_const_str(const std::string &tl_name);

vk::tl::type *get_tl_type_of_php_class(ClassPtr interface);
vk::tl::combinator *get_tl_constructor_of_php_class(ClassPtr klass);
std::vector<ClassPtr> get_all_php_classes_of_tl_constructor(const vk::tl::combinator *c);
std::vector<ClassPtr> get_all_php_classes_of_tl_type(const vk::tl::type *t);
ClassPtr get_php_class_of_tl_constructor_specialization(const vk::tl::combinator *c, const std::string &specialization_suffix);
ClassPtr get_php_class_of_tl_type_specialization(const vk::tl::type *t, const std::string &specialization_suffix);

ClassPtr get_php_class_of_tl_function(const vk::tl::combinator *f);
std::string get_tl_function_name_of_php_class(ClassPtr klass);
ClassPtr get_php_class_of_tl_function_result(const vk::tl::combinator *f);

vk::tl::type *get_this_from_renamed_tl_scheme(const vk::tl::type *t);
vk::tl::combinator *get_this_from_renamed_tl_scheme(const vk::tl::combinator *c);

std::vector<std::string> get_optional_args_for_call(const std::unique_ptr<vk::tl::combinator> &constructor);
std::vector<std::string> get_template_params(const vk::tl::combinator *constructor);
std::string get_template_declaration(const vk::tl::combinator *constructor);
std::string get_template_definition(const vk::tl::combinator *constructor);

std::string get_php_runtime_type(const vk::tl::combinator *c, bool wrap_to_class_instance = false, const std::string &type_name = "");
std::string get_php_runtime_type(const vk::tl::type *t);

bool is_php_class_a_tl_function(ClassPtr klass);
bool is_php_class_a_tl_constructor(ClassPtr klass);
bool is_php_class_a_tl_polymorphic_type(ClassPtr klass);
bool is_php_class_a_tl_array_item(ClassPtr klass);
bool is_tl_type_a_php_array(const vk::tl::type *t);
bool is_tl_type_wrapped_to_Optional(const vk::tl::type *type);

bool is_type_dependent(const vk::tl::type *type, const vk::tl::tl_scheme *);
bool is_type_dependent(const vk::tl::combinator *constructor, const vk::tl::tl_scheme *);
}
