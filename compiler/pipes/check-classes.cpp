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
  const auto *type_data = root->tinf_node.get_type();
  if (auto var = root.try_as<op_var>()) {
    type_data = var->var_id ? var->var_id->tinf_node.get_type() : type_data;
  }
  if (type_data && type_data->ptype() == tp_Class) {
    const auto &class_types = type_data->class_types();
    kphp_error(std::distance(class_types.begin(), class_types.end()) == 1,
               fmt_format("Can't deduce class type, possible options are: {}",
                          vk::join(class_types, ", ", [](ClassPtr c) { return c->as_human_readable(); }))
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
  if (klass->does_need_codegen()) {
    check_instance_fields_inited(klass);
  }
  if (klass->can_be_php_autoloaded && !klass->is_builtin() && !klass->need_generated_stub) {
    kphp_error(klass->file_id->main_function->body_seq == FunctionData::body_value::empty,
               fmt_format("class {} can be autoloaded, but its file contains some logic (maybe, require_once files with global vars?)\n",
                          klass->as_human_readable()));
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
               fmt_format("static {} is not inited at declaration (inferred {})",
                          f.var->as_human_readable(), f.get_inferred_type()->as_human_readable()));
  });
}

inline void CheckClassesPass::check_instance_fields_inited(ClassPtr klass) {
  klass->members.for_each([](const ClassMemberInstanceField &f) {
    PrimitiveType ptype = f.var->tinf_node.get_type()->get_real_ptype();
    kphp_error(ptype != tp_any,
               fmt_format("var {} is declared but never written; please, provide a default value", f.var->as_human_readable()));
  });
}

void CheckClassesPass::check_serialized_fields(ClassPtr klass) {
  used_serialization_tags_t used_serialization_tags_for_fields{false};
  fill_reserved_serialization_tags(used_serialization_tags_for_fields, klass);

  klass->members.for_each([&](ClassMemberInstanceField &f) {
    auto kphp_serialized_field_tag = f.phpdoc ? f.phpdoc->find_tag(PhpDocType::kphp_serialized_field) : nullptr;
    if (!kphp_serialized_field_tag) {
      kphp_error(!klass->is_serializable, fmt_format("kphp-serialized-field is required for field: {}", f.local_name()));
      return;
    }

    auto the_klass = klass;

    // This loop finishes unconditionally since there is NULL klass->parent_class if there is no base class.
    while (the_klass) {
      // Inheritance with serialization is allowed if
      // * parent class has ho instance field
      // * if there are instance fields, class should be marked with kphp-serializable
      kphp_error_return(
        (the_klass->members.has_any_instance_var() && the_klass->is_serializable) || (!the_klass->members.has_any_instance_var()),
        fmt_format("Class {} and all its ancestors must be @kphp-serializable if there are instance fields. Class {} is not.", klass->name, the_klass->name));
      the_klass = the_klass->parent_class;
    }

    if (kphp_serialized_field_tag->value.starts_with("none")) {
      return;
    }

    std::string value = kphp_serialized_field_tag->value_as_string();
    const auto *field_type = f.root->var_id->tinf_node.get_type();
    std::unordered_set<ClassPtr> classes_inside;
    field_type->get_all_class_types_inside(classes_inside);
    for (auto inner_c : classes_inside) {
      kphp_error(inner_c->is_serializable, fmt_format("class {} must be serializable as it is used in field {}::{}",
                                                      TermStringFormat::paint_red(inner_c->name), klass->name, f.local_name()));
    }

    try {
      size_t processed_pos{0};
      auto kphp_serialized_field = std::stoi(value, &processed_pos);
      if (processed_pos != value.size() && vk::none_of_equal(value[processed_pos], ' ', '/', '#', '\n')) {
        throw std::invalid_argument("");
      }
      if (kphp_serialized_field < 0 || max_serialization_tag_value <= kphp_serialized_field) {
        kphp_error_return(false, fmt_format("kphp-serialized-field({}) must be >=0 and < than {}", kphp_serialized_field, max_serialization_tag_value + 0));
      }
      if (used_serialization_tags_for_fields[kphp_serialized_field]) {
        kphp_error_return(false, fmt_format("kphp-serialized-field: {} is already in use", kphp_serialized_field));
      }
      f.serialization_tag = kphp_serialized_field;
      f.serialize_as_float32 = f.phpdoc->has_tag(PhpDocType::kphp_serialized_float32);

      auto f_tag = f.serialization_tag;
      the_klass = klass->parent_class;
      while (the_klass) {
        auto same_numbered_field = the_klass->members.find_member([&f_tag](const ClassMemberInstanceField &f) {
          return f.serialization_tag == f_tag;
        });
        if (same_numbered_field) {
          kphp_error_return(false, fmt_format("kphp-serialized-field: field with number {} found in both classes {} and {}", f_tag, the_klass->name, klass->name));
        }
        the_klass = the_klass->parent_class;
      }

      used_serialization_tags_for_fields[kphp_serialized_field] = true;
    } catch (std::logic_error &) {
      kphp_error_return(false, fmt_format("bad kphp-serialized-field: '{}'", value));
    }
  });

  klass->members.for_each([&](ClassMemberStaticField &f) {
    if (f.phpdoc) {
      kphp_error_return(!f.phpdoc->has_tag(PhpDocType::kphp_serialized_field) &&
                        !f.phpdoc->has_tag(PhpDocType::kphp_serialized_float32),
                        fmt_format("kphp-serialized-field is allowed only for instance fields: {}", f.local_name()));
    }
  });
}

void CheckClassesPass::fill_reserved_serialization_tags(used_serialization_tags_t &used_serialization_tags_for_fields, ClassPtr klass) {
  if (!klass->phpdoc) {
    return;
  }
  if (const PhpDocTag *reserved_ids = klass->phpdoc->find_tag(PhpDocType::kphp_reserved_fields)) {
    vk::string_view ids = reserved_ids->value;
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

