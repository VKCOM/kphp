// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/final-check.h"

#include "common/termformat/termformat.h"
#include "common/algorithms/string-algorithms.h"
#include "common/algorithms/contains.h"

#include "compiler/compiler-core.h"
#include "compiler/data/kphp-json-tags.h"
#include "compiler/data/kphp-tracing-tags.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/data/vars-collector.h"
#include "compiler/vertex-util.h"
#include "compiler/type-hint.h"

namespace {
void check_class_immutableness(ClassPtr klass) {
  if (!klass->is_immutable) {
    return;
  }
  klass->members.for_each([klass](const ClassMemberInstanceField &field) {
    kphp_assert(field.var->marked_as_const);
    std::unordered_set<ClassPtr> sub_classes;
    field.var->tinf_node.get_type()->get_all_class_types_inside(sub_classes);
    for (auto sub_class : sub_classes) {
      kphp_error(sub_class->is_immutable,
                 fmt_format("Field {} of immutable class {} should be immutable too, but class {} is mutable",
                            TermStringFormat::paint(std::string{field.local_name()}, TermStringFormat::red),
                            TermStringFormat::paint(klass->name, TermStringFormat::red),
                            TermStringFormat::paint(sub_class->name, TermStringFormat::red)));
    }
  });

  kphp_error(!klass->parent_class || klass->parent_class->is_immutable,
             fmt_format("Immutable class {} has mutable base {}",
                        TermStringFormat::paint(klass->name, TermStringFormat::red),
                        TermStringFormat::paint(klass->parent_class->name, TermStringFormat::red)));
}

void process_job_worker_class(ClassPtr klass) {
  auto request_interface = G->get_class("KphpJobWorkerRequest");
  auto response_interface = G->get_class("KphpJobWorkerResponse");
  auto shared_memory_piece_interface = G->get_class("KphpJobWorkerSharedMemoryPiece");
  if (!shared_memory_piece_interface) {   // when functions.txt deleted while development
    return;
  }

  bool implements_request = request_interface->is_parent_of(klass);
  bool implements_response = response_interface->is_parent_of(klass);
  bool implements_shared_memory_piece = shared_memory_piece_interface->is_parent_of(klass);

  if (implements_request || implements_response || implements_shared_memory_piece) {
    klass->deeply_require_instance_cache_visitor();
    klass->deeply_require_virtual_builtin_functions();
  }
  if (implements_shared_memory_piece) {
    kphp_error(klass->is_immutable, fmt_format("Class {} must be immutable (@kphp-immutable-class) as it implements KphpJobWorkerSharedMemoryPiece", klass->name));
  }
  if (implements_request) {
    std::vector<const ClassMemberInstanceField *> shared_memory_pieces_fields = klass->get_job_shared_memory_pieces();
    kphp_error(shared_memory_pieces_fields.size() <= 1, fmt_format("Class {} must have at most 1 member implementing KphpJobWorkerSharedMemoryPiece, but {} found", klass->name, shared_memory_pieces_fields.size()));
    klass->has_job_shared_memory_piece = !shared_memory_pieces_fields.empty();
  }
};

void check_instance_cache_fetch_call(VertexAdaptor<op_func_call> call) {
  auto klass = tinf::get_type(call)->class_type();
  kphp_assert(klass);
  klass->deeply_require_instance_cache_visitor();
  kphp_error(klass->is_immutable,
             fmt_format("Can not fetch instance of mutable class {} with instance_cache_fetch call", klass->name));
}

void check_instance_cache_store_call(VertexAdaptor<op_func_call> call) {
  const auto *type = tinf::get_type(call->args()[1]);
  kphp_error_return(type->ptype() == tp_Class, "Called instance_cache_store() with a non-instance argument");
  auto klass = type->class_type();
  klass->deeply_require_instance_cache_visitor();
  kphp_error(!klass->is_polymorphic_or_has_polymorphic_member(),
             "Can not store instance with interface inside with instance_cache_store call");
  kphp_error(klass->is_immutable,
             fmt_format("Can not store instance of mutable class {} with instance_cache_store call", klass->name));
}

void to_array_debug_on_class(ClassPtr klass) {
  kphp_error(!klass->is_ffi_cdata(), "Called to_array_debug() with CData");
  klass->deeply_require_to_array_debug_visitor();
}

void check_to_array_debug_call(VertexAdaptor<op_func_call> call) {
  const auto *type = tinf::get_type(call->args()[0]);
  const auto ptype = type->ptype();
  if (ptype == tp_Class) {
    to_array_debug_on_class(type->class_type());
  } else if (vk::any_of_equal(ptype, tp_tuple, tp_shape)) {
    std::unordered_set<ClassPtr> classes;
    type->get_all_class_types_inside(classes);
    for (const auto &klass : classes) {
      to_array_debug_on_class(klass);
    }
  } else {
    kphp_error_return(false, "Argument of to_array_debug() should be instance, tuple or shape");
  }
}

void check_field_of_a_jsonable_class(const ClassMemberInstanceField &field, const kphp_json::FieldJsonSettings &props, bool to_encode) noexcept {
  const TypeData *type = tinf::get_type(field.var);

  kphp_error_return(!props.array_as_hashmap || type->ptype() == tp_array,
                    fmt_format("Field {} is @kphp-json 'array_as_hashmap', but it's not an array", field.var->as_human_readable()));
  kphp_error_return(!props.raw_string || (type->ptype() == tp_string && !type->or_false_flag() && !type->or_null_flag()),
                    fmt_format("Field {} is @kphp-json 'raw_string', but it's not a string", field.var->as_human_readable()));

  if (field.type_hint) {
    field.type_hint->traverse([field, to_encode](const TypeHint *child) {
      // for now, these types are denied both for encoding and decoding,
      // though potentially tuples and shapes can easily be allowed at least for encoding
      bool denied = child->try_as<TypeHintTuple>() || child->try_as<TypeHintShape>() || child->try_as<TypeHintCallable>() || child->try_as<TypeHintFuture>();
      if (const auto *as_instance = child->try_as<TypeHintInstance>()) {
        ClassPtr field_class = as_instance->resolve();
        denied = field_class->is_builtin();

        if (!to_encode) {
          if (field_class->is_interface()) {
            kphp_error(0, fmt_format("Json decoding for {} is unavailable, because {} is an interface", field.var->as_human_readable(), field_class->as_human_readable()));
          } else if (field_class->modifiers.is_abstract()) {
            kphp_error(0, fmt_format("Json decoding for {} is unavailable, because {} is an abstract class", field.var->as_human_readable(), field_class->as_human_readable()));
          } else if (!field_class->derived_classes.empty()) {
            kphp_error(0, fmt_format("Json decoding for {} is unavailable, because {} has derived classes", field.var->as_human_readable(), field_class->as_human_readable()));
          }
        }
      }
      kphp_error(!denied, fmt_format("Field {} is @var {}, it's incompatible with json", field.var->as_human_readable(), field.type_hint->as_human_readable()));
    });
  }
}

// when we see JsonEncoder::encode($klass) and similar, append to klass->json_encoders
// (so, in on_start() that list is empty, and all checks are done when adding the next found at a call)
void store_json_encoder(ClassPtr klass, ClassPtr json_encoder, bool to_encode) noexcept {
  {
    AutoLocker<Lockable *> lock{&(*klass)};
    auto it = std::find(klass->json_encoders.begin(), klass->json_encoders.end(), std::pair{json_encoder, to_encode});
    if (it != klass->json_encoders.end()) {
      return;
    }
    // maintain a list sorted to keep codegen stable
    klass->json_encoders.emplace_front(json_encoder, to_encode);
    klass->json_encoders.sort();
  }

  // we allow encoding interfaces and base classes (just generate a virtual accept for all inheritors),
  // but for decoding we allow only leaf classes (it's conceptual, json strings don't contain class names)
  // check this and print an appropriate error message
  if (!to_encode) {
    if (klass->is_interface()) {
      kphp_error(0, fmt_format("Json decoding for {} is unavailable, because it's an interface", klass->as_human_readable()));
    } else if (klass->modifiers.is_abstract()) {
      kphp_error(0, fmt_format("Json decoding for {} is unavailable, because it's at abstract class", klass->as_human_readable()));
    }
  }

  for (ClassPtr derived : klass->derived_classes) {
    store_json_encoder(derived, json_encoder, to_encode);
  }

  std::set<std::string> used_json_keys;
  for (ClassPtr parent = klass; parent; parent = parent->parent_class) {
    parent->members.for_each([json_encoder, to_encode, klass, parent, &used_json_keys](const ClassMemberInstanceField &field) {
      // tuples/interfaces are not allowed as fields, but if a field is marked with @kphp-json skip
      // or is implicitly skipped (for example, due to visibility_policy of encoder),
      // we shouldn't analyze it and store json encoders for it
      // to correctly handle all cases, we merge all encoder constants and tags, equal to code generation
      kphp_json::FieldJsonSettings props = kphp_json::merge_and_inherit_json_tags(field, parent, json_encoder);
      if (to_encode ? props.skip_when_encoding : props.skip_when_decoding) {
        return;
      }

      std::unordered_set<ClassPtr> sub_classes;
      field.var->tinf_node.get_type()->get_all_class_types_inside(sub_classes);
      for (ClassPtr sub_klass : sub_classes) {
        store_json_encoder(sub_klass, json_encoder, to_encode);
      }

      check_field_of_a_jsonable_class(field, props, to_encode);
      kphp_error(props.json_key.empty() || used_json_keys.insert(props.json_key).second,
                 fmt_format("Json key \"{}\" appears twice for class {} encoded with {}", props.json_key, klass->as_human_readable(), json_encoder->as_human_readable()));
    });
  }
}

ClassPtr extract_json_encoder_from_call(VertexAdaptor<op_func_call> call) noexcept {
  auto v_encoder = VertexUtil::get_actual_value(call->args().front());
  kphp_assert(v_encoder->type() == op_string);  // it's const string of a class name (JsonEncoder of inheritors)
  return G->get_class(v_encoder->get_string());
}

void check_to_json_impl_call(VertexAdaptor<op_func_call> call) noexcept {
  auto encoder = extract_json_encoder_from_call(call);
  const auto *type = tinf::get_type(call->args()[1]);
  kphp_assert(type->ptype() == tp_Class);
  store_json_encoder(type->class_type(), encoder, true);
}

void check_from_json_impl_call(VertexAdaptor<op_func_call> call) noexcept {
  auto encoder = extract_json_encoder_from_call(call);
  const auto *type = tinf::get_type(call);
  kphp_assert(type->ptype() == tp_Class);
  store_json_encoder(type->class_type(), encoder, false);
}

void check_instance_serialize_call(VertexAdaptor<op_func_call> call) {
  const auto *type = tinf::get_type(call->args()[0]);
  kphp_error_return(type->ptype() == tp_Class, "Called instance_serialize() with a non-instance argument");
  kphp_error(type->class_type()->is_serializable, fmt_format("Called instance_serialize() for class {}, but it's not marked with @kphp-serializable", type->class_type()->name));
}

void check_instance_deserialize_call(VertexAdaptor<op_func_call> call) {
  const auto *type = tinf::get_type(call);
  kphp_assert(type->ptype() == tp_Class);
  kphp_error(type->class_type()->is_serializable, fmt_format("Called instance_deserialize() for class {}, but it's not marked with @kphp-serializable", type->class_type()->name));
}

void check_estimate_memory_usage_call(VertexAdaptor<op_func_call> call) {
  const auto *type = tinf::get_type(call->args()[0]);
  std::unordered_set<ClassPtr> classes_inside;
  type->get_all_class_types_inside(classes_inside);
  for (auto klass: classes_inside) {
    klass->deeply_require_instance_memory_estimate_visitor();
  }
}

void check_get_global_vars_memory_stats_call() {
  kphp_error_return(G->settings().enable_global_vars_memory_stats.get(),
                    "function get_global_vars_memory_stats() disabled, use KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS to enable");
}

static void check_kphp_tracing_func_enter_branch_call(FunctionPtr current_function) {
  kphp_error(current_function->kphp_tracing && current_function->kphp_tracing->is_aggregate(),
             "kphp_tracing_func_enter_branch() is available only inside functions with @kphp-tracing aggregate");
}

void check_register_shutdown_functions(VertexAdaptor<op_func_call> call) {
  auto callback = call->args()[0].as<op_callback_of_builtin>();
  if (!callback->func_id->can_throw()) {
    return;
  }
  std::vector<std::string> throws;
  for (const auto &e : callback->func_id->exceptions_thrown) {
    throws.emplace_back(e->name);
  }
  kphp_error(false,
             fmt_format("register_shutdown_callback should not throw exceptions\n"
                        "But it may throw {}\n"
                        "Throw chain: {}",
                        vk::join(throws, ", "), callback->func_id->get_throws_call_chain()));
}

void mark_global_vars_for_memory_stats() {
  if (!G->settings().enable_global_vars_memory_stats.get()) {
    return;
  }

  static std::atomic<bool> vars_marked{false};
  if (vars_marked.exchange(true)) {
    return;
  }

  std::unordered_set<ClassPtr> classes_inside;
  VarsCollector vars_collector{0, [&classes_inside](VarPtr variable) {
    tinf::get_type(variable)->get_all_class_types_inside(classes_inside);
    return false;
  }};
  vars_collector.collect_global_and_static_vars_from(G->get_main_file()->main_function);
  for (auto klass: classes_inside) {
    klass->deeply_require_instance_memory_estimate_visitor();
  }
}

void check_func_call_params(VertexAdaptor<op_func_call> call) {
  FunctionPtr f = call->func_id;
  VertexRange func_params = f->get_params();

  VertexRange call_params = call->args();
  int call_params_n = static_cast<int>(call_params.size());
  if (call_params_n != func_params.size()) {
    return;
  }

  for (int i = 0; i < call_params_n; i++) {
    auto param = func_params[i].as<op_func_param>();
    bool is_callback_passed_to_extern = f->is_extern() && param->type_hint && param->type_hint->try_as<TypeHintCallable>();
    if (!is_callback_passed_to_extern) {
      kphp_error(call_params[i]->type() != op_callback_of_builtin, "Unexpected function pointer");
      continue;
    }
    const auto *type_hint_callable = param->type_hint->try_as<TypeHintCallable>();

    if (call_params[i]->type() == op_ffi_php2c_conv) {
      continue;
    }

    auto callback_of_builtin = call_params[i].as<op_callback_of_builtin>();
    FunctionPtr f_passed_to_builtin = callback_of_builtin->func_id;
    if (f->class_id && f->class_id->ffi_scope_mixin) {
      kphp_error(!callback_of_builtin->func_id->can_throw(),
                 fmt_format("FFI callback should not throw\nThrow chain: {}", callback_of_builtin->func_id->get_throws_call_chain()));
      kphp_error(callback_of_builtin->size() == 0, "FFI callbacks should not capture any variables");
    }

    kphp_error(!f_passed_to_builtin->is_resumable, fmt_format("Callbacks passed to builtin functions must not be resumable.\n"
                                                              "But '{}' became resumable because of the calls chain:\n"
                                                              "{}", f_passed_to_builtin->as_human_readable(), f_passed_to_builtin->get_resumable_path()));

    if (auto name = f_passed_to_builtin->local_name(); name == "to_array_debug" || name == "instance_to_array") {
      if (const auto *as_subkey = type_hint_callable->arg_types[0]->try_as<TypeHintArgSubkeyGet>()) {
        const auto *arg_ref = as_subkey->inner->try_as<TypeHintArgRef>();
        if (auto arg = VertexUtil::get_call_arg_ref(arg_ref ? arg_ref->arg_num : -1, call)) {
          const auto *value_type = tinf::get_type(arg)->lookup_at_any_key();
          auto out_class = value_type->class_type();
          kphp_error_return(out_class, "type of argument for to_array_debug has to be array of Classes");
          out_class->deeply_require_to_array_debug_visitor();
        }
      }
    }
  }
}

void check_null_usage_in_binary_operations(VertexAdaptor<meta_op_binary> binary_vertex) {
  const auto *lhs_type = tinf::get_type(binary_vertex->lhs());
  const auto *rhs_type = tinf::get_type(binary_vertex->rhs());

  switch (binary_vertex->type()) {
    case op_add:
    case op_set_add:
      if (vk::any_of_equal(tp_array, lhs_type->get_real_ptype(), rhs_type->get_real_ptype())) {
        kphp_error(vk::none_of_equal(tp_any, lhs_type->ptype(), rhs_type->ptype()),
                   fmt_format("Can't use '{}' operation between {} and {} types",
                              OpInfo::str(binary_vertex->type()), lhs_type->as_human_readable(), rhs_type->as_human_readable()));
        return;
      }

      // fall through
    case op_mul:
    case op_sub:
    case op_div:
    case op_mod:
    case op_pow:
    case op_and:
    case op_or:
    case op_xor:
    case op_shl:
    case op_shr:

    case op_set_mul:
    case op_set_sub:
    case op_set_div:
    case op_set_mod:
    case op_set_pow:
    case op_set_and:
    case op_set_or:
    case op_set_xor:
    case op_set_shl:
    case op_set_shr: {
      kphp_error((lhs_type->ptype() != tp_any || lhs_type->or_false_flag()) &&
                 (rhs_type->ptype() != tp_any || rhs_type->or_false_flag()),
                 fmt_format("Got '{}' operation between {} and {} types",
                            OpInfo::str(binary_vertex->type()), lhs_type->as_human_readable(), rhs_type->as_human_readable()));
      return;
    }

    default:
      return;
  }
}

void check_function_throws(FunctionPtr f) {
  std::unordered_set<std::string> throws_expected(f->check_throws.begin(), f->check_throws.end());
  std::unordered_set<std::string> throws_actual;
  for (const auto &e : f->exceptions_thrown) {
    throws_actual.insert(e->name);
  }
  kphp_error(throws_expected == throws_actual,
             fmt_format("kphp-throws mismatch: have <{}>, want <{}>",
                        vk::join(throws_actual, ", "),
                        vk::join(throws_expected, ", ")));
}

void check_ffi_call(VertexAdaptor<op_func_call> call) {
  // right now this function contains only 1 check, but
  // I moved FFI-related checks to a separate function anyway

  if (call->get_string() == "FFI$$string" && call->args().size() == 1) {
    // it's not a part of a function signature constraints,
    // but PHP will give a run-time error for 1-argument form
    // of the function if it's not a `char*` or `const char*`;
    // we'll try to give that error at the compile-time
    const auto *type = tinf::get_type(call->args()[0]);
    int indirection = type->get_indirection();
    if (const auto *elem_type = type->lookup_at_any_key()) {
      type = elem_type;
      indirection = 1;
    }
    const auto *ffi_type = FFIRoot::get_ffi_type(type->class_type());
    kphp_error(indirection == 1 && ffi_type->kind == FFITypeKind::Char,
               fmt_format("$ptr argument is {}, expected a C string compatible type",
                          type->as_human_readable()));
    return;
  }
}

bool is_php2c_valid(VertexAdaptor<op_ffi_php2c_conv> conv, const FFIType *ffi_type, const TypeData *php_type) {
  auto php_expr = conv->expr();

  if (php_type->use_or_false()) {
    return false;
  }

  if (ffi_type->is_cstring() && !php_type->use_or_null()) {
    // auto casting from PHP string to `const char*` is allowed in simple contexts
    if (conv->simple_dst && php_type->ptype() == tp_string) {
      return true;
    }
  }

  switch (ffi_type->kind) {
    case FFITypeKind::Int8:
    case FFITypeKind::Int16:
    case FFITypeKind::Int32:
    case FFITypeKind::Int64:
    case FFITypeKind::Uint8:
    case FFITypeKind::Uint16:
    case FFITypeKind::Uint32:
    case FFITypeKind::Uint64:
      return php_type->ptype() == tp_int;

    case FFITypeKind::Char:
      return php_type->ptype() == tp_string && !php_type->use_or_null();

    case FFITypeKind::Bool:
      return vk::any_of_equal(php_expr->type(), op_false, op_true) || php_type->ptype() == tp_bool;

    case FFITypeKind::Float:
    case FFITypeKind::Double:
      return php_type->ptype() == tp_float;

    case FFITypeKind::Struct:
    case FFITypeKind::StructDef:
    case FFITypeKind::Union:
    case FFITypeKind::UnionDef:
      return php_type->class_type() == conv->c_type->to_type_data()->class_type();

    case FFITypeKind::Pointer:
      if (php_expr->type() == op_null) {
        return true;
      }
      if (!ffi_type->members[0]->is_const() && php_type->ffi_const_flag()) {
        return false;
      }
      if (ffi_type->members[0]->kind == FFITypeKind::Void && ffi_type->num == 1) {
        return true;
      }
      if (ffi_type->num == php_type->get_indirection()) {
        return php_type->class_type() == conv->c_type->to_type_data()->class_type();
      }
      return false;

    default:
      return false;
  }
}

void check_php2c_conv(VertexAdaptor<op_ffi_php2c_conv> conv) {
  if (const auto *as_instance = conv->c_type->try_as<TypeHintInstance>()) {
    if (as_instance->full_class_name == "FFI\\CData") {
      return;
    }
  }
  if (conv->c_type->try_as<TypeHintCallable>() && conv->expr()->type() == op_null) {
    return;
  }
  const auto *php_type = tinf::get_type(conv->expr());
  const FFIType *ffi_type = FFIRoot::get_ffi_type(conv->c_type);
  kphp_error(is_php2c_valid(conv, ffi_type, php_type),
             fmt_format("Invalid php2c conversion{}: {} -> {}",
                        conv->simple_dst ? "" : " in this context",
                        php_type->as_human_readable(),
                        ffi_decltype_string(ffi_type)));
}

} // namespace

void FinalCheckPass::on_start() {
  mark_global_vars_for_memory_stats();

  if (current_function->type == FunctionData::func_class_holder) {
    check_class_immutableness(current_function->class_id);
    process_job_worker_class(current_function->class_id);
  }

  check_magic_methods(current_function);

  if (current_function->should_not_throw && current_function->can_throw()) {
    kphp_error(0, fmt_format("Function {} marked as @kphp-should-not-throw, but really can throw an exception:\n{}",
                             current_function->as_human_readable(), current_function->get_throws_call_chain()));
  }

  for (auto &static_var : current_function->static_var_ids) {
    check_static_var_inited(static_var);
  }

  if (current_function->kphp_lib_export) {
    check_lib_exported_function(current_function);
  }

  if (!current_function->check_throws.empty()) {
    check_function_throws(current_function);
  }
}

VertexPtr FinalCheckPass::on_enter_vertex(VertexPtr vertex) {
  if (vertex->type() == op_func_name) {
    kphp_error (0, fmt_format("Unexpected {} (maybe, it should be a define?)", vertex->get_string()));
  }
  if (vertex->type() == op_addr) {
    kphp_error (0, "Getting references is unsupported");
  }
  if (vertex->type() == op_ffi_php2c_conv) {
    check_php2c_conv(vertex.as<op_ffi_php2c_conv>());
  }
  if (auto array_get = vertex.try_as<op_ffi_array_get>()) {
    const auto *key_type = tinf::get_type(array_get->key());
    kphp_error(key_type->ptype() == tp_int,
               fmt_format("ffi_array_get index type must be int, {} used instead", key_type->as_human_readable()));
  }if (auto array_set = vertex.try_as<op_ffi_array_set>()) {
    const auto *key_type = tinf::get_type(array_set->key());
    kphp_error(key_type->ptype() == tp_int,
               fmt_format("ffi_array_set index type must be int, {} used instead", key_type->as_human_readable()));
  }

  if (vertex->type() == op_eq3) {
    check_eq3(vertex.as<meta_op_binary>()->lhs(), vertex.as<meta_op_binary>()->rhs());
  }
  if (vk::any_of_equal(vertex->type(), op_lt, op_le, op_spaceship, op_eq2)) {
    check_comparisons(vertex.as<meta_op_binary>()->lhs(), vertex.as<meta_op_binary>()->rhs(), vertex->type());
  }
  if (vertex->type() == op_add) {
    const TypeData *type_left = tinf::get_type(vertex.as<meta_op_binary>()->lhs());
    const TypeData *type_right = tinf::get_type(vertex.as<meta_op_binary>()->rhs());
    if ((type_left->ptype() == tp_array) ^ (type_right->ptype() == tp_array)) {
      if (type_left->ptype() != tp_mixed && type_right->ptype() != tp_mixed) {
        kphp_warning (fmt_format("{} + {} is strange operation", type_out(type_left), type_out(type_right)));
      }
    }
  }
  if (vk::any_of_equal(vertex->type(), op_sub, op_mul, op_div, op_mod, op_pow)) {
    const TypeData *type_left = tinf::get_type(vertex.as<meta_op_binary>()->lhs());
    const TypeData *type_right = tinf::get_type(vertex.as<meta_op_binary>()->rhs());
    if ((type_left->ptype() == tp_array) || (type_right->ptype() == tp_array)) {
      kphp_warning(fmt_format("{} {} {} is strange operation",
                              OpInfo::str(vertex->type()),
                              type_out(type_left),
                              type_out(type_right)));
    }
  }

  if (vertex->type() == op_foreach) {
    VertexPtr arr = vertex.as<op_foreach>()->params()->xs();
    const TypeData *arrayType = tinf::get_type(arr);
    if (arrayType->ptype() == tp_array) {
      const TypeData *valueType = arrayType->lookup_at_any_key();
      if (valueType->get_real_ptype() == tp_any) {
        kphp_error (0, "Can not compile foreach on array of Unknown type");
      }
    }
  }
  if (vertex->type() == op_list) {
    const auto list = vertex.as<op_list>();
    VertexPtr arr = list->array();
    const TypeData *arrayType = tinf::get_type(arr);
    if (arrayType->ptype() == tp_array) {
      const TypeData *valueType = arrayType->lookup_at_any_key();
      kphp_error (valueType->get_real_ptype() != tp_any, "Can not compile list with array of Unknown type");
    } else if (arrayType->ptype() == tp_tuple) {
      size_t list_size = vertex.as<op_list>()->list().size();
      size_t tuple_size = arrayType->get_tuple_max_index();
      kphp_error (list_size <= tuple_size, fmt_format("Can't assign tuple of length {} to list of length {}", tuple_size, list_size));
      for (auto cur : list->list()) {
        const auto kv = cur.as<op_list_keyval>();
        if (VertexUtil::get_actual_value(kv->key())->type() != op_int_const) {
          const TypeData *key_type = tinf::get_type(kv->key());
          kphp_error(0, fmt_format("Only int const keys can be used, got '{}'", ptype_name(key_type->ptype())));
        }
      }
    } else if (arrayType->ptype() == tp_shape) {
      for (auto cur : list->list()) {
        const auto kv = cur.as<op_list_keyval>();
        if (VertexUtil::get_actual_value(kv->key())->type() != op_string) {
          const TypeData *key_type = tinf::get_type(kv->key());
          kphp_error(0, fmt_format("Only string const keys can be used, got '{}'", ptype_name(key_type->ptype())));
        }
      }
    } else {
      kphp_error (arrayType->ptype() == tp_mixed, fmt_format("Can not compile list with '{}'", ptype_name(arrayType->ptype())));
    }
  }
  if (vertex->type() == op_index && vertex.as<op_index>()->has_key()) {
    VertexPtr arr = vertex.as<op_index>()->array();
    VertexPtr key = vertex.as<op_index>()->key();
    const TypeData *array_type = tinf::get_type(arr);
    // TODO: do we need this?
    if (array_type->ptype() == tp_tuple) {
      const auto key_value = VertexUtil::get_actual_value(key);
      if (key_value->type() == op_int_const) {
        const long index = parse_int_from_string(key_value.as<op_int_const>());
        const size_t tuple_size = array_type->get_tuple_max_index();
        kphp_error(0 <= index && index < tuple_size, fmt_format("Can't get element {} of tuple of length {}", index, tuple_size));
      }
    }
    const TypeData *key_type = tinf::get_type(key);
    kphp_error(key_type->ptype() != tp_any || key_type->or_false_flag(),
               fmt_format("Can't get array element by key with {} type", key_type->as_human_readable()));
  }
  if (auto xset = vertex.try_as<meta_op_xset>()) {
    auto v = xset->expr();
    if (auto var_vertex = v.try_as<op_var>()) {    // isset($var), unset($var)
      VarPtr var = var_vertex->var_id;
      if (vertex->type() == op_unset) {
        kphp_error(!var->is_reference, "Unset of reference variables is not supported");
        if (var->is_in_global_scope()) {
          if (current_function->type != FunctionData::func_main && current_function->type != FunctionData::func_switch) {
            kphp_error(0, "Unset of global variables in functions is not supported");
          }
        }
      } else {
        const TypeData *type_info = tinf::get_type(var);
        kphp_error(type_info->can_store_null(),
                   fmt_format("isset({}) will be always true for {}", var->as_human_readable(), type_info->as_human_readable()));
      }
    } else if (v->type() == op_index) {   // isset($arr[index]), unset($arr[index])
      const TypeData *arrayType = tinf::get_type(v.as<op_index>()->array());
      PrimitiveType ptype = arrayType->get_real_ptype();
      kphp_error(vk::any_of_equal(ptype, tp_tuple, tp_shape, tp_array, tp_mixed), "Can't use isset/unset by[idx] for not an array");
    }
  }
  if (vertex->type() == op_func_call) {
    check_op_func_call(vertex.as<op_func_call>());
  }
  if (vertex->type() == op_return && current_function->is_no_return) {
    kphp_error(false, "Return is done from no return function");
  }
  if (current_function->can_throw() && current_function->is_no_return) {
    kphp_error(false, "Exception is thrown from no return function");
  }
  if (vertex->type() == op_instance_prop) {
    const TypeData *lhs_type = tinf::get_type(vertex.as<op_instance_prop>()->instance());
    kphp_error(lhs_type->ptype() == tp_Class,
               fmt_format("Accessing ->property of non-instance {}", lhs_type->as_human_readable()));
  }

  if (vertex->type() == op_throw) {
    const TypeData *thrown_type = tinf::get_type(vertex.as<op_throw>()->exception());
    kphp_error(thrown_type->ptype() == tp_Class && G->get_class("Throwable")->is_parent_of(thrown_type->class_type()),
               fmt_format("Throw not Throwable, but {}", thrown_type->as_human_readable()));
  }

  if (vertex->type() == op_fork) {
    const VertexAdaptor<op_func_call> &func_call = vertex.as<op_fork>()->func_call();
    kphp_error(!func_call->func_id->is_extern(), "fork of builtin function is forbidden");
    kphp_error(tinf::get_type(func_call)->get_real_ptype() != tp_void, "forking void functions is forbidden, return null at least");
  }

  if (G->settings().warnings_level.get() >= 2 && vertex->type() == op_func_call) {
    FunctionPtr function_where_require = current_function;

    if (function_where_require && function_where_require->type == FunctionData::func_local) {
      FunctionPtr function_which_required = vertex.as<op_func_call>()->func_id;
      if (function_which_required->is_main_function()) {
        for (VarPtr global_var : function_which_required->global_var_ids) {
          if (!global_var->marked_as_global) {
            kphp_warning(fmt_format("require file with global variable not marked as global: {}", global_var->name));
          }
        }
      }
    }
  }

  if (auto binary_vertex = vertex.try_as<meta_op_binary>()) {
    check_null_usage_in_binary_operations(binary_vertex);
  }

  if (vertex->type() == op_instanceof) {
    check_instanceof(vertex.try_as<op_instanceof>());
  }

  if (auto v_index = vertex.try_as<op_index>()) {
    check_indexing(v_index->array(), v_index->key());
  } else if (auto v_set = vertex.try_as<op_set_value>()) {
    check_indexing(v_set->array(), v_set->key());
  } else if (auto v_array = vertex.try_as<op_array>()) {
    check_array_literal(v_array);
  }

  //TODO: may be this should be moved to tinf_check
  return vertex;
}

void FinalCheckPass::check_instanceof(VertexAdaptor<op_instanceof> instanceof_vertex) {
  const auto instance_var = instanceof_vertex->lhs();
  if (!instance_var) {
    return;
  }

  const auto *instanceof_var_type = tinf::get_type(instance_var);
  if (!instanceof_var_type) {
    return;
  }

  kphp_error(instanceof_var_type->class_type(), fmt_format("left operand of 'instanceof' should be an instance, but passed {}", instanceof_var_type->as_human_readable()));
}

static void check_indexing_violation(vk::string_view allowed_types_string, const std::vector<PrimitiveType> &allowed_types, vk::string_view what_indexing,
                                     const TypeData &key_type) noexcept {
  kphp_error(vk::contains(allowed_types, key_type.ptype()),
             fmt_format("Only {} types are allowed for {}{}indexing, but {} type is passed", allowed_types_string, what_indexing,
                        what_indexing.empty() ? "" : " ", key_type.as_human_readable()));
}

static void check_indexing_type(PrimitiveType indexed_type, const TypeData &key_type) noexcept {
  if (key_type.get_real_ptype() == tp_bool) {
    return;
  }

  if (indexed_type == tp_tuple || indexed_type == tp_string) {
    static const std::vector allowed_types{tp_int, tp_mixed, tp_future};
    check_indexing_violation("int and future", allowed_types, ptype_name(indexed_type), key_type);
  } else {
    static const std::vector allowed_types{tp_int, tp_float, tp_string, tp_tmp_string, tp_mixed, tp_future};
    check_indexing_violation("int, float, string and future", allowed_types, {}, key_type);
  }
}

void FinalCheckPass::check_array_literal(VertexAdaptor<op_array> vertex) noexcept {
  for (const auto &element : vertex->args()) {
    if (element->type() == op_double_arrow) {
      auto arrow = element.as<op_double_arrow>();
      if (const auto *key_type = tinf::get_type(arrow->key())) {
        check_indexing_type(tp_array, *key_type);
      }
    }
  }
}

void FinalCheckPass::check_indexing(VertexPtr array, VertexPtr key) noexcept {
  const auto *key_type = tinf::get_type(key);
  const auto *array_type = tinf::get_type(array);
  if (key_type || array_type) {
    check_indexing_type(array_type->ptype(), *key_type);
  }
}

VertexPtr FinalCheckPass::on_exit_vertex(VertexPtr vertex) {
  return vertex;
}

void FinalCheckPass::check_op_func_call(VertexAdaptor<op_func_call> call) {
  if (call->func_id->is_extern()) {
    const auto &function_name = call->get_string();
    if (function_name == "instance_cache_fetch") {
      check_instance_cache_fetch_call(call);
    } else if (function_name == "instance_cache_store") {
      check_instance_cache_store_call(call);
    } else if (function_name == "to_array_debug" || function_name == "instance_to_array") {
      check_to_array_debug_call(call);
    } else if (function_name == "JsonEncoder$$to_json_impl") {
      check_to_json_impl_call(call);
    } else if (function_name == "JsonEncoder$$from_json_impl") {
      check_from_json_impl_call(call);
    } else if (function_name == "instance_serialize") {
      check_instance_serialize_call(call);
    } else if (function_name == "instance_deserialize") {
      check_instance_deserialize_call(call);
    } else if (function_name == "estimate_memory_usage") {
      check_estimate_memory_usage_call(call);
    } else if (function_name == "get_global_vars_memory_stats") {
      check_get_global_vars_memory_stats_call();
    } else if (function_name == "kphp_tracing_func_enter_branch") {
      check_kphp_tracing_func_enter_branch_call(current_function);
    } else if (function_name == "is_null") {
      const TypeData *arg_type = tinf::get_type(call->args()[0]);
      kphp_error(arg_type->can_store_null(), fmt_format("is_null() will be always false for {}", arg_type->as_human_readable()));
    } else if (function_name == "register_shutdown_function") {
      check_register_shutdown_functions(call);
    } else if (vk::string_view{function_name}.starts_with("rpc_tl_query")) {
      G->set_untyped_rpc_tl_used();
    } else if (vk::string_view{function_name}.starts_with("FFI$$")) {
      check_ffi_call(call);
    }

    // TODO: express the array<Comparable> requirement in functions.txt and remove these adhoc checks?
    bool is_value_sort_function = vk::any_of_equal(function_name, "sort", "rsort", "asort", "arsort");
    if (is_value_sort_function) {
      // Forbid arrays with elements that would be rejected by check_comparisons().
      const TypeData *array_type = tinf::get_type(call->args()[0]);
      const auto *elem_type = array_type->lookup_at_any_key();
      kphp_error(vk::none_of_equal(elem_type->ptype(), tp_Class, tp_tuple, tp_shape),
                 fmt_format("{} is not comparable and cannot be sorted", elem_type->as_human_readable()));
    }
  }

  check_func_call_params(call);
}

// Inspection: static-var should be initialized at the declaration (with the exception of tp_mixed).
inline void FinalCheckPass::check_static_var_inited(VarPtr static_var) {
  kphp_error(static_var->init_val || tinf::get_type(static_var)->ptype() == tp_mixed,
             fmt_format("static ${} is not inited at declaration (inferred {})", static_var->name, tinf::get_type(static_var)->as_human_readable()));
}

void FinalCheckPass::check_lib_exported_function(FunctionPtr function) {
  const TypeData *ret_type = tinf::get_type(function, -1);
  kphp_error(!ret_type->has_class_type_inside(),
             "Can not use class instance in return of @kphp-lib-export function");

  for (auto p: function->get_params()) {
    auto param = p.as<op_func_param>();
    if (param->has_default_value() && param->default_value()) {
      VertexPtr default_value = VertexUtil::get_actual_value(param->default_value());
      kphp_error_act(vk::any_of_equal(default_value->type(), op_int_const, op_float_const),
                     "Only const int, const float are allowed as default param for @kphp-lib-export function",
                     continue);
    }
    kphp_error(!tinf::get_type(p)->has_class_type_inside(),
               "Can not use class instance in param of @kphp-lib-export function");
  }
}

void FinalCheckPass::check_eq3(VertexPtr lhs, VertexPtr rhs) {
  const auto *lhs_type = tinf::get_type(lhs);
  const auto *rhs_type = tinf::get_type(rhs);

  if ((lhs_type->ptype() == tp_float && !lhs_type->or_false_flag() && !lhs_type->or_null_flag()) ||
      (rhs_type->ptype() == tp_float && !rhs_type->or_false_flag() && !rhs_type->or_null_flag())) {
    kphp_warning(fmt_format("Identity operator between {} and {} types may give an unexpected result",
                            lhs_type->as_human_readable(), rhs_type->as_human_readable()));
  } else if (!can_be_same_type(lhs_type, rhs_type)) {
    kphp_warning(fmt_format("Types {} and {} can't be identical",
                            lhs_type->as_human_readable(), rhs_type->as_human_readable()));
  }
}

void FinalCheckPass::check_comparisons(VertexPtr lhs, VertexPtr rhs, Operation op) {
  if (vk::none_of_equal(tinf::get_type(lhs)->ptype(), tp_Class, tp_array, tp_tuple, tp_shape)) {
    std::swap(lhs, rhs);
  }

  const auto *lhs_t = tinf::get_type(lhs);
  const auto *rhs_t = tinf::get_type(rhs);

  if (lhs_t->ptype() == tp_Class) {
    if (op == op_eq2) {
      kphp_error(false, fmt_format("instance {} {} is unsupported", OpInfo::desc(op), rhs_t->as_human_readable()));
    } else {
      kphp_error(false, fmt_format("comparison instance with {} is prohibited (operation: {})", rhs_t->as_human_readable(), OpInfo::desc(op)));
    }
  } else if (lhs_t->ptype() == tp_array) {
    kphp_error(vk::any_of_equal(rhs_t->get_real_ptype(), tp_array, tp_bool, tp_mixed),
               fmt_format("{} is always > than {} used operator {}", lhs_t->as_human_readable(), rhs_t->as_human_readable(), OpInfo::desc(op)));
  } else if (lhs_t->ptype() == tp_tuple) {
    bool can_compare_with_tuple = op == op_eq2 && rhs_t->ptype() == tp_tuple && lhs_t->get_tuple_max_index() == rhs_t->get_tuple_max_index();
    kphp_error(can_compare_with_tuple,
               fmt_format("You may not compare {} with {} used operator {}", lhs_t->as_human_readable(), rhs_t->as_human_readable(), OpInfo::desc(op)));
  } else if (lhs_t->ptype() == tp_shape) {
    // shape can't be compared with anything using ==, it's meaningless
    kphp_error(0,
               fmt_format("You may not compare {} with {} used operator {}", lhs_t->as_human_readable(), rhs_t->as_human_readable(), OpInfo::desc(op)));
  }

}

bool FinalCheckPass::user_recursion(VertexPtr v) {
  if (v->type() == op_function) {
    run_function_pass(v.as<op_function>()->cmd_ref(), this);
    on_function();
    return true;
  }

  if (vk::any_of_equal(v->type(), op_func_call, op_var, op_index, op_conv_drop_false, op_conv_drop_null)) {
    if (v->rl_type == val_r) {
      const TypeData *type = tinf::get_type(v);
      if (type->get_real_ptype() == tp_any) {
        raise_error_using_Unknown_type(v);
        return true;
      }
    }
  }

  return false;
}

void FinalCheckPass::on_function() {
  const TypeData *return_type = tinf::get_type(current_function, -1);
  if (return_type->get_real_ptype() == tp_void) {
    kphp_error(!return_type->or_false_flag(), fmt_format("Function returns void and false simultaneously"));
    kphp_error(!return_type->or_null_flag(), fmt_format("Function returns void and null simultaneously"));
  }
  for (int i = 0; i < current_function->param_ids.size(); ++i) {
    const TypeData *arg_type = tinf::get_type(current_function, i);
    if (arg_type->get_real_ptype() == tp_any) {
      stage::set_location(current_function->root->location);
      raise_error_using_Unknown_type(current_function->get_params()[i].as<op_func_param>()->var());
    }
  }
}

void FinalCheckPass::raise_error_using_Unknown_type(VertexPtr v) {
  std::string index_depth;
  while (auto v_index = v.try_as<op_index>()) {
    v = v_index->array();
    index_depth += "[*]";
  }

  while (vk::any_of_equal(v->type(), op_conv_drop_false, op_conv_drop_null)) {
    v = v.try_as<meta_op_unary>()->expr();
  }

  if (auto var_vertex = v.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    if (index_depth.empty()) {              // Unknown type single var access
      kphp_error(0, fmt_format("Variable ${} has Unknown type", var->name));
    } else if (index_depth.size() == 3) {   // 1-depth array[*] access (most common case)

      if (vk::any_of_equal(tinf::get_type(var)->get_real_ptype(), tp_array, tp_mixed)) {
        kphp_error(0, fmt_format("Array ${} is always empty, getting its element has Unknown type", var->name));
      } else if (tinf::get_type(var)->get_real_ptype() == tp_shape) {
        kphp_error(0, fmt_format("Accessing unexisting element of shape ${}", var->name));
      } else {
        kphp_error(0, fmt_format("${} is {}, can not get element", var->name, tinf::get_type(var)->as_human_readable()));
      }

    } else {                                // multidimentional array[*]...[*] access
      kphp_error(0, fmt_format("${}{} has Unknown type", var->name, index_depth));
    }
  } else if (auto call = v.try_as<op_func_call>()) {
    if (index_depth.empty()) {
      kphp_error(0, fmt_format("Function {}() returns Unknown type", call->func_id->name));
    } else {
      kphp_error(0, fmt_format("{}(){} has Unknown type", call->func_id->name, index_depth));
    }
  } else {
    kphp_error(0, "Using Unknown type");
  }
}

void FinalCheckPass::check_magic_methods(FunctionPtr fun) {
  if (!fun->modifiers.is_instance()) {
    return;
  }

  const auto name = fun->local_name();

  if (name == ClassData::NAME_OF_TO_STRING) {
    check_magic_tostring_method(fun);
  } else if (name == ClassData::NAME_OF_CLONE) {
    check_magic_clone_method(fun);
  }
}

void FinalCheckPass::check_magic_tostring_method(FunctionPtr fun) {
  const auto count_args = fun->param_ids.size();
  kphp_error(count_args == 1, fmt_format("Magic method {} cannot take arguments", fun->as_human_readable()));

  const auto *ret_type = tinf::get_type(fun, -1);
  if (!ret_type) {
    return;
  }

  kphp_error(ret_type->ptype() == tp_string && ret_type->flags() == 0,
             fmt_format("Magic method {} must have string return type", fun->as_human_readable()));
}

void FinalCheckPass::check_magic_clone_method(FunctionPtr fun) {
  kphp_error(!fun->is_resumable, fmt_format("{} method has to be not resumable", fun->as_human_readable()));
  kphp_error(!fun->can_throw(), fmt_format("{} method should not throw exception", fun->as_human_readable()));
}
