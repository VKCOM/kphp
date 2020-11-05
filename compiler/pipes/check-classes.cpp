// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-data.h"
#include "compiler/phpdoc.h"

VertexPtr CheckClassesPass::on_enter_vertex(VertexPtr root) {
  auto type_data = root->tinf_node.type_;
  if (auto var = root.try_as<op_var>()) {
    type_data = var->var_id ? var->var_id->tinf_node.type_ : type_data;
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
  if (ClassData::does_need_codegen(klass)) {
    check_instance_fields_inited(klass);
  }
  if (klass->can_be_php_autoloaded && !klass->is_builtin()) {
    kphp_error(klass->file_id->main_function->body_seq == FunctionData::body_value::empty,
               fmt_format("class {} can be autoloaded, but its file contains some logic (maybe, require_once files with global vars?)\n",
                          klass->name));
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
                          klass->name, f.local_name(), colored_type_out(f.get_inferred_type())));
  });
}

inline void CheckClassesPass::check_instance_fields_inited(ClassPtr klass) {
  // TODO KPHP-221: the old code is kept for now (check for Unknown)
  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    PrimitiveType ptype = f.var->tinf_node.get_type()->get_real_ptype();
    kphp_error(ptype != tp_Unknown,
               fmt_format("var {}::${} is declared but never written; please, provide a default value", klass->name, f.local_name()));
  });
}

void CheckClassesPass::check_serialized_fields(ClassPtr klass) {
  used_serialization_tags_t used_serialization_tags_for_fields{false};
  fill_reserved_serialization_tags(used_serialization_tags_for_fields, klass);

  klass->members.for_each([&](ClassMemberInstanceField &f) {
    if (auto kphp_serialized_field_str = phpdoc_find_tag_as_string(f.phpdoc_str, php_doc_tag::kphp_serialized_field)) {
      if (!vk::string_view(*kphp_serialized_field_str).starts_with("none")) {
        kphp_error_return(klass->is_serializable, fmt_format("you may not use @kphp-serialized-field inside non-serializable klass: {}", klass->name));
        try {
          size_t processed_pos{0};
          auto kphp_serialized_field = std::stoi(*kphp_serialized_field_str, &processed_pos);
          if (processed_pos != kphp_serialized_field_str->size() && vk::none_of_equal((*kphp_serialized_field_str)[processed_pos], ' ', '/', '#', '\n')) {
            throw std::invalid_argument("");
          }
          kphp_error_return(0 <= kphp_serialized_field && kphp_serialized_field < max_serialization_tag_value, fmt_format("kphp-serialized-field({}) must be >=0 and < than {}", kphp_serialized_field, max_serialization_tag_value + 0));
          kphp_error_return(!used_serialization_tags_for_fields[kphp_serialized_field], fmt_format("kphp-serialized-field: {} is already in use", kphp_serialized_field));
          f.serialization_tag = kphp_serialized_field;
          used_serialization_tags_for_fields[kphp_serialized_field] = true;
        } catch (std::logic_error &) {
          kphp_error_return(false, fmt_format("bad kphp-serialized-field: '{}'", *kphp_serialized_field_str));
        }
      }
    } else {
      kphp_error_return(!klass->is_serializable, fmt_format("kphp-serialized-field is required for field: {}", f.local_name()));
    }
  });

  klass->members.for_each([&](ClassMemberStaticField &f) {
    kphp_error_return(!phpdoc_tag_exists(f.phpdoc_str, php_doc_tag::kphp_serialized_field),
                      fmt_format("kphp-serialized-field is allowed only for instance fields: {}", f.local_name()));
  });
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

