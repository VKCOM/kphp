// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/type-hint.h"

#include <mutex>

#include "common/php-functions.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/ffi-data.h"
#include "compiler/lambda-utils.h"
#include "compiler/name-gen.h"


/**
 * This class stores a big hashtable [hash => TypeHint]
 * Every TypeHint*::create() method at first looks up here, and allocates an object only if not found.
 * That's why all allocated TypeHint objects are unique, making flags and typedata once-calculation reliable.
 */
class HasherOfTypeHintForOptimization {
  uint64_t cur_hash;

  static TSHashTable<const TypeHint *> all_type_hints_ht;

public:
  explicit HasherOfTypeHintForOptimization(uint64_t hash_of_type_hint_class_name)
    : cur_hash(hash_of_type_hint_class_name) {}

  void feed_hash(uint64_t val) {
    cur_hash = cur_hash * 56235515617499ULL + val;
  }

  void feed_string(const std::string &s) {
    feed_hash(string_hash(s.c_str(), s.size()));
  }

  void feed_inner(const TypeHint *inner) {
    feed_hash(inner->hash);
  }

  const TypeHint *get_existing() const __attribute__((flatten)) {
    const auto *result = all_type_hints_ht.find(cur_hash);
    return result ? *result : nullptr;
  }

  const TypeHint *add_because_doesnt_exist(TypeHint *newly_created) const __attribute__((noinline)) {
    TSHashTable<const TypeHint *>::HTNode *node = all_type_hints_ht.at(cur_hash);
    AutoLocker<Lockable *> locker(node);
    if (!node->data) {
      newly_created->hash = cur_hash;
      newly_created->traverse([&newly_created](const TypeHint *child) {
        newly_created->flags |= child->flags; // parent's flags are merged child flags (which were already calculated up to here)
      });
      node->data = newly_created;
    } else {
      delete newly_created;
    }
    // in case you want to check for hash collisions, make get_existing() return nullptr and uncomment this line:
    // else kphp_assert(node->data->as_human_readable() == newly_created->as_human_readable());
    return node->data;
  }
};


TSHashTable<const TypeHint *> HasherOfTypeHintForOptimization::all_type_hints_ht;


// --------------------------------------------


const TypeData *TypeHint::to_type_data() const {
  kphp_assert(is_typedata_constexpr());

  if (!cached_typedata_if_constexpr) {
    TypeData *dst = TypeData::get_type(tp_any)->clone();
    recalc_type_data_in_context_of_call(dst, {}); // call = {}, as constexpr recalculation will never access it
    cached_typedata_if_constexpr = dst;
  }
  return cached_typedata_if_constexpr;
}

const TypeHint *TypeHint::unwrap_optional() const {
  if (const auto *as_optional = this->try_as<TypeHintOptional>()) {
    return as_optional->inner;
  }
  return this;
}

ClassPtr TypeHintInstance::resolve_and_set_klass() const {
  kphp_assert(!has_self_static_parent_inside());
  return klass = G->get_class(full_class_name);
}

const TypeHint *TypeHintRefToField::resolve_field_type_hint() const {
  if (const auto *inner_instance = inner->unwrap_optional()->try_as<TypeHintInstance>()) {
    if (ClassPtr inner_resolved = inner_instance->resolve()) {
      if (const ClassMemberInstanceField *member = inner_resolved->get_instance_field(field_name)) {
        return member->type_hint ?: TypeHintPrimitive::create(tp_any);
      }
      if (const ClassMemberStaticField *member = inner_resolved->get_static_field(field_name)) {
        return member->type_hint ?: TypeHintPrimitive::create(tp_any);
      }
    }
  }
  return nullptr;
}

FunctionPtr TypeHintRefToMethod::resolve_method() const {
  if (const auto *inner_instance = inner->unwrap_optional()->try_as<TypeHintInstance>()) {
    if (ClassPtr inner_resolved = inner_instance->resolve()) {
      if (const ClassMemberInstanceMethod *member = inner_resolved->get_instance_method(method_name)) {
        return member->function;
      }
      if (const ClassMemberStaticMethod *member = inner_resolved->members.get_static_method(method_name)) {
        return member->function;
      }
    }
  }
  return {};
}

InterfacePtr TypeHintCallable::get_interface() const {
  if (!interface) {
    kphp_assert(is_typedata_constexpr());
    interface = generate_interface_from_typed_callable(this);
  }
  return interface;
}

ClassPtr TypeHintCallable::get_lambda_class() const {
  return f_bound_to && f_bound_to->is_lambda() && f_bound_to->class_id && f_bound_to->class_id->is_lambda_class() ? f_bound_to->class_id : ClassPtr{};
}

const TypeHint *TypeHintShape::find_at(const std::string &key) const {
  for (const auto &p : items) {
    if (p.first == key) {
      return p.second;
    }
  }
  return nullptr;
}

std::map<std::int64_t, std::string> TypeHintShape::shape_keys_storage_;

void TypeHintShape::register_known_key(std::int64_t key_hash, std::string_view key) noexcept {
  static std::mutex mutex;

  std::lock_guard lock{mutex};
  shape_keys_storage_.emplace(key_hash, key);
}

const std::map<std::int64_t, std::string> &TypeHintShape::get_all_registered_keys() noexcept {
  return shape_keys_storage_;
}

// --------------------------------------------
//    create()
// all constructors of TypeHint classes are private, only TypeHint*::create() is allowed
// each create() method calculates hash and creates an object only if it isn't found in a global hashtable


const TypeHint *TypeHintArgRef::create(int arg_num) {
  HasherOfTypeHintForOptimization hash(5693590762732189562ULL);
  hash.feed_hash(arg_num);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintArgRef(arg_num)
  );
}

const TypeHint *TypeHintArgRefCallbackCall::create(int arg_num) {
  HasherOfTypeHintForOptimization hash(1296495626775553170ULL);
  hash.feed_hash(arg_num);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintArgRefCallbackCall(arg_num)
  );
}

const TypeHint *TypeHintArgRefInstance::create(int arg_num) {
  HasherOfTypeHintForOptimization hash(17043732264461707628ULL);
  hash.feed_hash(arg_num);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintArgRefInstance(arg_num)
  );
}

const TypeHint *TypeHintArgSubkeyGet::create(const TypeHint *inner) {
  HasherOfTypeHintForOptimization hash(1151996677232147726ULL);
  hash.feed_inner(inner);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintArgSubkeyGet(inner)
  );
}

const TypeHint *TypeHintArray::create(const TypeHint *inner) {
  HasherOfTypeHintForOptimization hash(7584642663608918592ULL);
  hash.feed_inner(inner);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintArray(inner)
  );
}

const TypeHint *TypeHintArray::create_array_of_any() {
  HasherOfTypeHintForOptimization hash(7584642663608918592ULL);
  hash.feed_inner(TypeHintPrimitive::create(tp_any));

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintArray(TypeHintPrimitive::create(tp_any))
  );
}

const TypeHint *TypeHintCallable::create(std::vector<const TypeHint *> &&arg_types, const TypeHint *return_type) {
  HasherOfTypeHintForOptimization hash(16112178357011888472ULL);
  for (const auto *arg : arg_types) {
    hash.feed_inner(arg);
    hash.feed_hash(161121);
  }
  hash.feed_inner(return_type);
  hash.feed_hash(161122);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintCallable(std::move(arg_types), return_type)
  );
}

const TypeHint *TypeHintCallable::create_untyped_callable() {
  HasherOfTypeHintForOptimization hash(16112178357011888472ULL);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintCallable()
  );
}

const TypeHint *TypeHintCallable::create_ptr_to_function(FunctionPtr f_bound_to) {
  HasherOfTypeHintForOptimization hash(16112178357011888472ULL);
  hash.feed_string(f_bound_to->name);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintCallable(f_bound_to)
  );
}

static void calculate_ffi_type_hash(HasherOfTypeHintForOptimization &hash, const FFIType *type) {
  hash.feed_hash(static_cast<uint64_t>(type->kind));
  hash.feed_hash(type->flags);
  hash.feed_hash(type->num);
  hash.feed_string(type->str);
  for (const FFIType *member : type->members) {
    calculate_ffi_type_hash(hash, member);
  }
}

const TypeHint *TypeHintFFIType::create(const std::string &scope_name, const FFIType *type, bool move) {
  HasherOfTypeHintForOptimization hash(4941189930887759468ULL);

  hash.feed_string(scope_name);
  calculate_ffi_type_hash(hash, type);

  if (const auto *existing = hash.get_existing()) {
    if (move) {
      // this cleanup is only necessary due to how we parse/create FFI types from the phpdoc
      ffi_type_delete(type);
    }
    return existing;
  }
  return hash.add_because_doesnt_exist(new TypeHintFFIType{scope_name, type});
}

const TypeHint *TypeHintFFIScopeArgRef::create(int arg_num) {
  HasherOfTypeHintForOptimization hash(9731618202711445786ULL);
  hash.feed_hash(arg_num);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintFFIScopeArgRef{arg_num}
  );
}

const TypeHint *TypeHintFFIScope::create(const std::string &scope_name) {
  HasherOfTypeHintForOptimization hash(8265450633645347407ULL);
  hash.feed_string(scope_name);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintFFIScope{scope_name}
  );
}

const TypeHint *TypeHintFuture::create(PrimitiveType ptype, const TypeHint *inner) {
  HasherOfTypeHintForOptimization hash(5072702167144640720ULL);
  hash.feed_inner(inner);
  hash.feed_hash(static_cast<uint64_t>(ptype));

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintFuture(ptype, inner)
  );
}

const TypeHint *TypeHintNotNull::create(const TypeHint *inner, bool drop_not_null, bool drop_not_false) {
  HasherOfTypeHintForOptimization hash(5192347813149011894ULL);
  hash.feed_inner(inner);
  hash.feed_hash(static_cast<uint64_t>(drop_not_null));
  hash.feed_hash(static_cast<uint64_t>(drop_not_false));

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintNotNull(inner, drop_not_null, drop_not_false)
  );
}

const TypeHint *TypeHintInstance::create(const std::string &full_class_name) {
  HasherOfTypeHintForOptimization hash(16370122391586404558ULL);
  hash.feed_string(full_class_name);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintInstance(full_class_name, is_string_self_static_parent(full_class_name))
  );
}

const TypeHint *TypeHintRefToField::create(const TypeHint *inner, const std::string &field_name) {
  HasherOfTypeHintForOptimization hash(12470101183576506574ULL);
  hash.feed_inner(inner);
  hash.feed_string(field_name);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintRefToField(inner, field_name)
  );
}

const TypeHint *TypeHintRefToMethod::create(const TypeHint *inner, const std::string &method_name) {
  HasherOfTypeHintForOptimization hash(11681103389463249563ULL);
  hash.feed_inner(inner);
  hash.feed_string(method_name);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintRefToMethod(inner, method_name)
  );
}

const TypeHint *TypeHintOptional::create(const TypeHint *inner, bool or_null, bool or_false) {
  if (const auto *opt = inner->try_as<TypeHintOptional>()) {
    return TypeHintOptional::create(opt->inner, opt->or_null || or_null, opt->or_false || or_false);
  }

  HasherOfTypeHintForOptimization hash(16016792735423539538ULL);
  hash.feed_inner(inner);
  hash.feed_hash(static_cast<int>(or_null) * 2 + static_cast<int>(or_false));

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintOptional(inner, or_null, or_false)
  );
}

const TypeHint *TypeHintPipe::create(std::vector<const TypeHint *> &&items) {
  HasherOfTypeHintForOptimization hash(420035265936859756ULL);
  for (const TypeHint *item : items) {
    hash.feed_inner(item);
    hash.feed_hash(420035);
  }

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintPipe(std::move(items))
  );
}

const TypeHint *TypeHintPrimitive::create(PrimitiveType ptype) {
  HasherOfTypeHintForOptimization hash(18266006284047729844ULL);
  hash.feed_hash(static_cast<uint64_t>(ptype));

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintPrimitive(ptype)
  );
}

const TypeHint *TypeHintObject::create() {
  HasherOfTypeHintForOptimization hash(13255191682146639421ULL);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintObject()
  );
}

const TypeHint *TypeHintShape::create(std::vector<std::pair<std::string, const TypeHint *>> &&items, bool is_vararg) {
  HasherOfTypeHintForOptimization hash(8163025479511413046ULL);
  for (const auto &item : items) {
    hash.feed_string(item.first);
    hash.feed_inner(item.second);
    hash.feed_hash(816302);
  }
  if (is_vararg) {
    hash.feed_hash(1);
  }

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintShape(std::move(items), is_vararg)
  );
}

const TypeHint *TypeHintTuple::create(std::vector<const TypeHint *> &&items) {
  HasherOfTypeHintForOptimization hash(8195157858190991427ULL);
  for (const TypeHint *item : items) {
    hash.feed_inner(item);
    hash.feed_hash(819515);
  }

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintTuple(std::move(items))
  );
}

const TypeHint *TypeHintGenericT::create(const std::string &nameT) {
  HasherOfTypeHintForOptimization hash(4117413899192632779ULL);
  hash.feed_string(nameT);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintGenericT(nameT)
  );
}

const TypeHint *TypeHintClassString::create(const TypeHint *inner) {
  HasherOfTypeHintForOptimization hash(2914531788341198112ULL);
  hash.feed_inner(inner);

  return hash.get_existing() ?: hash.add_because_doesnt_exist(
    new TypeHintClassString(inner)
  );
}


// --------------------------------------------
//    as_human_readable()
// returns a notation very similar to how it's expressed in phpdoc


std::string TypeHintArgRef::as_human_readable() const {
  return std::string("^") + std::to_string(arg_num);
}

std::string TypeHintArgRefCallbackCall::as_human_readable() const {
  return "^" + std::to_string(arg_num) + "()";
}

std::string TypeHintArgRefInstance::as_human_readable() const {
  return "instance<^" + std::to_string(arg_num) + ">";
}

std::string TypeHintArgSubkeyGet::as_human_readable() const {
  return inner->as_human_readable() + "[*]";
}

std::string TypeHintArray::as_human_readable() const {
  std::string inner_str = inner->as_human_readable();
  bool embrace = inner->try_as<TypeHintOptional>() || inner->try_as<TypeHintCallable>() || inner->try_as<TypeHintPipe>();
  return embrace ? "(" + inner_str + ")" + "[]" : inner_str + "[]";
}

std::string TypeHintCallable::as_human_readable() const {
  return is_untyped_callable()
         ? f_bound_to ? f_bound_to->name : "callable"
         : "callable(" + vk::join(arg_types, ", ", [](const TypeHint *arg) { return arg->as_human_readable(); }) + "): " + return_type->as_human_readable();
}

std::string TypeHintFFIType::as_human_readable() const {
  return "ffi_cdata<" + scope_name + ", " + ffi_decltype_string(type) + ">";
}

std::string TypeHintFFIScopeArgRef::as_human_readable() const {
  return "ffi_scope<^" + std::to_string(arg_num) + ">";
}

std::string TypeHintFFIScope::as_human_readable() const {
  return "ffi_scope<" + scope_name + ">";
}

std::string TypeHintFuture::as_human_readable() const {
  return std::string(ptype_name(ptype)) + "<" + inner->as_human_readable() + ">";
}

std::string TypeHintNotNull::as_human_readable() const {
  return std::string(drop_not_null ? "not_null" : "") + (drop_not_false ? "not_false" : "") + "<" + inner->as_human_readable() + ">";
}

std::string TypeHintInstance::as_human_readable() const {
  ClassPtr resolved = resolve();
  return resolved ? resolved->as_human_readable() : full_class_name;
}

std::string TypeHintRefToField::as_human_readable() const {
  return inner->as_human_readable() + "::" + field_name;
}

std::string TypeHintRefToMethod::as_human_readable() const {
  return inner->as_human_readable() + "::" + method_name + "()";
}

std::string TypeHintOptional::as_human_readable() const {
  return or_null && !or_false ? "?" + inner->as_human_readable() : inner->as_human_readable() + (or_null ? "|null" : "") + (or_false ? "|false" : "");
}

std::string TypeHintPipe::as_human_readable() const {
  return vk::join(items, "|", [](const TypeHint *item) { return item->as_human_readable(); });
}

std::string TypeHintPrimitive::as_human_readable() const {
  return ptype_name(ptype);
}

std::string TypeHintObject::as_human_readable() const {
  return "object";
}

std::string TypeHintShape::as_human_readable() const {
  return "shape(" + vk::join(items, ", ", [](const auto &item) { return item.first + ":" + item.second->as_human_readable(); }) + (is_vararg ? ", ...)" : ")");
}

std::string TypeHintTuple::as_human_readable() const {
  return "tuple(" + vk::join(items, ", ", [](const TypeHint *item) { return item->as_human_readable(); }) + ")";
}

std::string TypeHintGenericT::as_human_readable() const {
  return nameT;
}

std::string TypeHintClassString::as_human_readable() const {
  return "class_string<" + inner->as_human_readable() + ">";
}


// --------------------------------------------
//    traverse()
// invokes a callback for type hint itself and all its children


void TypeHintArgRef::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintArgRefCallbackCall::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintArgRefInstance::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintArgSubkeyGet::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}

void TypeHintArray::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}

void TypeHintCallable::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  for (const TypeHint *arg : arg_types) {
    arg->traverse(callback);
  }
  if (return_type != nullptr) {
    return_type->traverse(callback);
  }
}

void TypeHintFFIType::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintFFIScopeArgRef::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintFFIScope::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintFuture::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}

void TypeHintNotNull::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}

void TypeHintInstance::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintRefToField::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}

void TypeHintRefToMethod::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}

void TypeHintOptional::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}

void TypeHintPipe::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  for (const TypeHint *item : items) {
    item->traverse(callback);
  }
}

void TypeHintPrimitive::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintObject::traverse(const TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintShape::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  for (const auto &item : items) {
    item.second->traverse(callback);
  }
}

void TypeHintTuple::traverse(const TraverserCallbackT &callback) const {
  callback(this);
  for (const TypeHint *item : items) {
    item->traverse(callback);
  }
}

void TypeHintGenericT::traverse(const TypeHint::TraverserCallbackT &callback) const {
  callback(this);
}

void TypeHintClassString::traverse(const TypeHint::TraverserCallbackT &callback) const {
  callback(this);
  inner->traverse(callback);
}


// --------------------------------------------
//    replace_self_static_parent()
// returns a new type hint with resolved self/static/parent inside
// for example, tuple(self[], int) becomes tuple(T[], int)
// it's called externally only if a flag is set, so we don't need to do extra checks inside implementations


const TypeHint *TypeHintArgRef::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintArgRefCallbackCall::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintArgRefInstance::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintArgSubkeyGet::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintArray::replace_self_static_parent(FunctionPtr resolve_context) const {
  return create(inner->replace_self_static_parent(resolve_context));
}

const TypeHint *TypeHintCallable::replace_self_static_parent(FunctionPtr resolve_context) const {
  if (is_untyped_callable()) {
    return this;
  }
  auto mapper = vk::make_transform_iterator_range([resolve_context](auto sub) { return sub->replace_self_static_parent(resolve_context); }, arg_types.begin(), arg_types.end());
  return create({mapper.begin(), mapper.end()}, return_type->replace_self_static_parent(resolve_context));
}

const TypeHint *TypeHintFFIType::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintFFIScopeArgRef::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintFFIScope::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintFuture::replace_self_static_parent(FunctionPtr resolve_context) const {
  return create(ptype, inner->replace_self_static_parent(resolve_context));
}

const TypeHint *TypeHintNotNull::replace_self_static_parent(FunctionPtr resolve_context) const {
  return create(inner->replace_self_static_parent(resolve_context), drop_not_null, drop_not_false);
}

const TypeHint *TypeHintInstance::replace_self_static_parent(FunctionPtr resolve_context) const {
  return is_string_self_static_parent(full_class_name) ? create(resolve_uses(resolve_context, full_class_name)) : this;
}

const TypeHint *TypeHintRefToField::replace_self_static_parent(FunctionPtr resolve_context) const {
  return create(inner->replace_self_static_parent(resolve_context), field_name);
}

const TypeHint *TypeHintRefToMethod::replace_self_static_parent(FunctionPtr resolve_context) const {
  return create(inner->replace_self_static_parent(resolve_context), method_name);
}

const TypeHint *TypeHintOptional::replace_self_static_parent(FunctionPtr resolve_context) const {
  return create(inner->replace_self_static_parent(resolve_context), or_null, or_false);
}

const TypeHint *TypeHintPipe::replace_self_static_parent(FunctionPtr resolve_context) const {
  auto mapper = vk::make_transform_iterator_range([resolve_context](auto sub) { return sub->replace_self_static_parent(resolve_context); }, items.begin(), items.end());
  return create({mapper.begin(), mapper.end()});
}

const TypeHint *TypeHintPrimitive::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintObject::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintShape::replace_self_static_parent(FunctionPtr resolve_context) const {
  auto mapper = vk::make_transform_iterator_range([resolve_context](const auto &sub) { return std::make_pair(sub.first, sub.second->replace_self_static_parent(resolve_context)); }, items.begin(), items.end());
  return create({mapper.begin(), mapper.end()}, is_vararg);
}

const TypeHint *TypeHintTuple::replace_self_static_parent(FunctionPtr resolve_context) const {
  auto mapper = vk::make_transform_iterator_range([resolve_context](auto sub) { return sub->replace_self_static_parent(resolve_context); }, items.begin(), items.end());
  return create({mapper.begin(), mapper.end()});
}

const TypeHint *TypeHintGenericT::replace_self_static_parent(FunctionPtr resolve_context __attribute__ ((unused))) const {
  return this;
}

const TypeHint *TypeHintClassString::replace_self_static_parent(FunctionPtr resolve_context) const {
  return create(inner->replace_self_static_parent(resolve_context));
}


// --------------------------------------------
//    replace_children_custom()
// returns a new type hint with children replaced by a custom callback
// used to replace ^1 and other argrefs with assumptions — for example, ^1[] could become A[]
// also used to replace generic T with an exact type hint — to convert tuple(T) to tuple(B)
// replace_self_static_parent() could be dropped off, but is left for performance


const TypeHint *TypeHintArgRef::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintArgRefCallbackCall::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintArgRefInstance::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintArgSubkeyGet::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintArray::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(create(inner->replace_children_custom(callback)));
}

const TypeHint *TypeHintCallable::replace_children_custom(const ReplacerCallbackT &callback) const {
  if (is_untyped_callable()) {
    return callback(this);
  }
  auto mapper = vk::make_transform_iterator_range([callback](auto sub) { return sub->replace_children_custom(callback); }, arg_types.begin(), arg_types.end());
  return callback(create({mapper.begin(), mapper.end()}, return_type->replace_children_custom(callback)));
}

const TypeHint *TypeHintFFIType::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintFFIScopeArgRef::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintFFIScope::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintFuture::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(create(ptype, inner->replace_children_custom(callback)));
}

const TypeHint *TypeHintNotNull::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(create(inner->replace_children_custom(callback), drop_not_null, drop_not_false));
}

const TypeHint *TypeHintInstance::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintRefToField::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(create(inner->replace_children_custom(callback), field_name));
}

const TypeHint *TypeHintRefToMethod::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(create(inner->replace_children_custom(callback), method_name));
}

const TypeHint *TypeHintOptional::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(create(inner->replace_children_custom(callback), or_null, or_false));
}

const TypeHint *TypeHintPipe::replace_children_custom(const ReplacerCallbackT &callback) const {
  auto mapper = vk::make_transform_iterator_range([callback](auto sub) { return sub->replace_children_custom(callback); }, items.begin(), items.end());
  return callback(create({mapper.begin(), mapper.end()}));
}

const TypeHint *TypeHintPrimitive::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintObject::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintShape::replace_children_custom(const ReplacerCallbackT &callback) const {
  auto mapper = vk::make_transform_iterator_range([callback](const auto &sub) { return std::make_pair(sub.first, sub.second->replace_children_custom(callback)); }, items.begin(), items.end());
  return callback(create({mapper.begin(), mapper.end()}, is_vararg));
}

const TypeHint *TypeHintTuple::replace_children_custom(const ReplacerCallbackT &callback) const {
  auto mapper = vk::make_transform_iterator_range([callback](auto sub) { return sub->replace_children_custom(callback); }, items.begin(), items.end());
  return callback(create({mapper.begin(), mapper.end()}));
}

const TypeHint *TypeHintGenericT::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(this);
}

const TypeHint *TypeHintClassString::replace_children_custom(const ReplacerCallbackT &callback) const {
  return callback(create(inner->replace_children_custom(callback)));
}


// --------------------------------------------
//    recalc_type_data_in_context_of_call()
// they are implemented in a separate file: see compiler/inferring/type-hint-recalc.cpp
