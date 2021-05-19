// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-classes.h"

#include "common/termformat/termformat.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-data.h"
#include "compiler/phpdoc.h"

VertexPtr CheckClassesPass::on_enter_vertex(VertexPtr root) {
  auto type_data = root->tinf_node.get_type();
  if (auto var = root.try_as<op_var>()) {
    type_data = var->var_id ? var->var_id->tinf_node.get_type() : type_data;
  }
  if (type_data && type_data->ptype() == tp_Class) {
    const auto &class_types = type_data->class_types();
    kphp_error(std::distance(class_types.begin(), class_types.end()) == 1,
               fmt_format("Can't deduce class type, possible options are: {}",
                          vk::join(class_types, ", ", [](ClassPtr c) { return c->src_name; }))
    );
  }

  return root;
}

void CheckClassesPass::on_finish() {
  if (current_function->type == FunctionData::func_class_holder) {
    analyze_class(current_function->class_id);
  }
}

inline void CheckClassesPass::analyze_class(ClassPtr klass) {
  check_static_fields_inited(klass);
  check_serialized_fields(klass);
  check_magic_methods(klass);
  if (ClassData::does_need_codegen(klass)) {
    check_instance_fields_inited(klass);
  }
  if (klass->can_be_php_autoloaded && !klass->is_builtin()) {
    kphp_error(klass->file_id->main_function->body_seq == FunctionData::body_value::empty,
               fmt_format("class {} can be autoloaded, but its file contains some logic (maybe, require_once files with global vars?)\n",
                          klass->name));
  }
  if (klass->is_serializable) {
    kphp_error(!klass->parent_class || !klass->parent_class->members.has_any_instance_var(),
               "You may not serialize classes which has a parent with fields");
  }
}

/*
 * Check that all static class fields are properly initialized during the declaration.
 */
inline void CheckClassesPass::check_static_fields_inited(ClassPtr klass) {
  klass->members.for_each([&](const ClassMemberStaticField &f) {
    bool allow_no_default_value = false;
    // default value can be omitted for the instance values
    if (!f.var->init_val) {
      allow_no_default_value = vk::any_of_equal(f.get_inferred_type()->ptype(), tp_Class);
    }

    kphp_error(f.var->init_val || allow_no_default_value,
               fmt_format("static {}::${} is not inited at declaration (inferred {})",
                          klass->name, f.local_name(), f.get_inferred_type()->as_human_readable()));
  });
}

inline void CheckClassesPass::check_instance_fields_inited(ClassPtr klass) {
  // TODO KPHP-221: the old code is kept for now (check for Unknown)
  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    PrimitiveType ptype = f.var->tinf_node.get_type()->get_real_ptype();
    kphp_error(ptype != tp_any,
               fmt_format("var {}::${} is declared but never written; please, provide a default value", klass->name, f.local_name()));
  });
}

void CheckClassesPass::check_serialized_fields(ClassPtr klass) {
  used_serialization_tags_t used_serialization_tags_for_fields{false};
  fill_reserved_serialization_tags(used_serialization_tags_for_fields, klass);

  klass->members.for_each([&](ClassMemberInstanceField &f) {
    auto kphp_serialized_field_str = phpdoc_find_tag_as_string(f.phpdoc_str, php_doc_tag::kphp_serialized_field);
    if (!kphp_serialized_field_str) {
      kphp_error(!klass->is_serializable, fmt_format("kphp-serialized-field is required for field: {}", f.local_name()));
      return;
    }

    kphp_error_return(klass->is_serializable, fmt_format("you may not use @kphp-serialized-field inside non-serializable klass: {}", klass->name));
    if (vk::string_view(*kphp_serialized_field_str).starts_with("none")) {
      return;
    }

    const auto *field_type = f.root->var_id->tinf_node.get_type();
    std::unordered_set<ClassPtr> classes_inside;
    field_type->get_all_class_types_inside(classes_inside);
    for (auto inner_c : classes_inside) {
      kphp_error(inner_c->is_serializable, fmt_format("class {} must be serializable as it is used in field {}::{}",
                                                      TermStringFormat::paint_red(inner_c->name), klass->name, f.local_name()));
    }

    try {
      size_t processed_pos{0};
      auto kphp_serialized_field = std::stoi(*kphp_serialized_field_str, &processed_pos);
      if (processed_pos != kphp_serialized_field_str->size() && vk::none_of_equal((*kphp_serialized_field_str)[processed_pos], ' ', '/', '#', '\n')) {
        throw std::invalid_argument("");
      }
      if (kphp_serialized_field < 0 || max_serialization_tag_value <= kphp_serialized_field) {
        kphp_error_return(false, fmt_format("kphp-serialized-field({}) must be >=0 and < than {}", kphp_serialized_field, max_serialization_tag_value + 0));
      }
      if (used_serialization_tags_for_fields[kphp_serialized_field]) {
        kphp_error_return(false, fmt_format("kphp-serialized-field: {} is already in use", kphp_serialized_field));
      }
      f.serialization_tag = kphp_serialized_field;
      f.serialize_as_float32 = phpdoc_tag_exists(f.phpdoc_str, php_doc_tag::kphp_serialized_float32);
      used_serialization_tags_for_fields[kphp_serialized_field] = true;
    } catch (std::logic_error &) {
      kphp_error_return(false, fmt_format("bad kphp-serialized-field: '{}'", *kphp_serialized_field_str));
    }
  });

  klass->members.for_each([&](ClassMemberStaticField &f) {
    kphp_error_return(!phpdoc_tag_exists(f.phpdoc_str, php_doc_tag::kphp_serialized_field) &&
                      !phpdoc_tag_exists(f.phpdoc_str, php_doc_tag::kphp_serialized_float32),
                      fmt_format("kphp-serialized-field is allowed only for instance fields: {}", f.local_name()));
  });
}

void CheckClassesPass::check_magic_methods(ClassPtr klass) {
  klass->members.for_each([&](const ClassMemberInstanceMethod &f) {
    if (vk::ends_with(f.function->name, ClassData::NAME_OF_TO_STRING)) {
      check_magic_tostring_method(f.function);
    }
  });
}

void CheckClassesPass::check_magic_tostring_method(FunctionPtr fun) {
  stage::set_function(fun);

  const auto count_args = fun->param_ids.size();
  kphp_error(count_args == 1, fmt_format("Magic method {} cannot take arguments", fun->get_human_readable_name()));

  const auto *ret_type = tinf::get_type(fun, -1);
  if (!ret_type) {
    return;
  }

  kphp_error(ret_type->ptype() == tp_string, fmt_format("Magic method {} must have string return type", fun->get_human_readable_name()));
}

void CheckClassesPass::fill_reserved_serialization_tags(used_serialization_tags_t &used_serialization_tags_for_fields, ClassPtr klass) {
  if (auto reserved_ids = phpdoc_find_tag_as_string(klass->phpdoc_str, php_doc_tag::kphp_reserved_fields)) {
    vk::string_view ids(*reserved_ids);
    kphp_error_return(ids.size() >= 2 && ids[0] == '[' && ids[ids.size() - 1] == ']', "reserved tags must be wrapped by []");
    ids = ids.substr(1, ids.size() - 2);

    for (auto id_str : split_skipping_delimeters(ids, ",")) {
      int id;
      kphp_error_return(sscanf(id_str.data(), "%d", &id) == 1, "tag expected");
      kphp_error_return(0 <= id && id < max_serialization_tag_value, fmt_format("kphp-reserved-field({}) must be >=0 and < than {}", id, max_serialization_tag_value + 0));
      used_serialization_tags_for_fields[id] = true;
    }
  }
}

