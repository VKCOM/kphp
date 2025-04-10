// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "compiler/compiler-core.h"
#include "compiler/debug.h"
#include "compiler/inferring/primitive-type.h"

// do not confuse TypeHint with TypeData!
// TypeData is a part of _type inferring_; it's mutable and plain, it represents current inferred state of every vertex
class TypeData;

/*
 * TypeHint is a user-given phpdoc representation
 * 'int', '?A[]', 'array|mixed', 'callable():void', 'tuple(^1[*])' — all are type hints
 * In PHP code, type hints can be specified by @var/@param/@return or arguments/fields type hints since PHP 7 (primitives only)
 * Unlike TypeData (which is flat), TypeHint is a tree: '?int[]' is TypeHintOptional > TypeHintArray > TypeHintPrimitive
 * Instances of TypeHint are supposed to be read only, but fields are public (I don't like getters)
 * Only unique type hints are created: thus, 'int|false' occurred in many php files, is actually created only once
 *
 * TypeHint can be converted to TypeData if it is _constexpr_:
 * * if it contains ^1 inside, it's not constexpr, as it requires a context of a call
 * * if it contains self inside, it's not constexpr, as it requires a resolve context
 * * if it contains T inside (of a @kphp-generic function), it's not constexpr, as it requires a generic instantiation
 * * otherwise, it's constexpr
 * Note, that self/static/parent are valid type hints (instances), but only while parsing (inheritance, traits, etc)
 * When bound to a function, they must be replaced with actual context, see phpdoc_finalize_type_hint_and_resolve()
 */
class TypeHint {
  DEBUG_STRING_METHOD {
    return as_human_readable();
  }

  // these fields are calculated only once for every unique type hint
  uint64_t hash{0}; // a unique hash: only unique type hints are actually created
  int flags{0};     // bits of flag_mask, to store often-used properties and return them without tree traversing

  // to leave public interface more clear, don't expose setters, use friend
  friend class HasherOfTypeHintForOptimization;

  // this field is calculated only once on need, see to_type_data()
  mutable const TypeData* cached_typedata_if_constexpr{nullptr};

protected:
  enum flag_mask {
    flag_contains_instances_inside = 1 << 0,
    flag_contains_self_static_parent_inside = 1 << 1,
    flag_contains_argref_inside = 1 << 2,
    flag_contains_tp_any_inside = 1 << 3,
    flag_contains_callables_inside = 1 << 4,
    flag_contains_genericT_inside = 1 << 5,
    flag_contains_autogeneric_inside = 1 << 6,
    flag_potentially_casts_rhs = 1 << 10,
  };

  explicit TypeHint(int self_flags_without_children)
      : flags(self_flags_without_children) {}

public:
  virtual ~TypeHint() = default; // though instances of TypeHint are never deleted :)

  template<class Derived>
  const Derived* try_as() const {
    return dynamic_cast<const Derived*>(this);
  }

  bool has_instances_inside() const {
    return flags & flag_contains_instances_inside;
  }
  bool has_self_static_parent_inside() const {
    return flags & flag_contains_self_static_parent_inside;
  }
  bool has_argref_inside() const {
    return flags & flag_contains_argref_inside;
  }
  bool has_tp_any_inside() const {
    return flags & flag_contains_tp_any_inside;
  }
  bool has_callables_inside() const {
    return flags & flag_contains_callables_inside;
  }
  bool has_genericT_inside() const {
    return flags & flag_contains_genericT_inside;
  }
  bool has_autogeneric_inside() const {
    return flags & flag_contains_autogeneric_inside;
  }
  bool has_flag_maybe_casts_rhs() const {
    return flags & flag_potentially_casts_rhs;
  }

  bool is_typedata_constexpr() const {
    return !has_argref_inside() && !has_self_static_parent_inside() && !has_genericT_inside();
  }
  const TypeData* to_type_data() const;
  const TypeHint* unwrap_optional() const;

  using TraverserCallbackT = std::function<void(const TypeHint* child)>;
  using ReplacerCallbackT = std::function<const TypeHint*(const TypeHint* child)>;

  virtual std::string as_human_readable() const = 0;
  virtual void traverse(const TraverserCallbackT& callback) const = 0;
  virtual const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const = 0;
  virtual const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const = 0;
  virtual void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const = 0;
};

/**
 * ^1, ^2 — references to types of other arguments per the exact call, used in functions.txt
 * For example, 'function eq(any $x) : ^1' returns the same type as the argument
 */
class TypeHintArgRef : public TypeHint {
  explicit TypeHintArgRef(int arg_num)
      : TypeHint(flag_contains_argref_inside),
        arg_num(arg_num) {}

public:
  int arg_num;

  static const TypeHint* create(int arg_num);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * ^1(), ^2() — references of callback calls of other arguments, used in functions.txt
 * For example, array_map (callback(...), $array) : ^1() []
 */
class TypeHintArgRefCallbackCall : public TypeHint {
  explicit TypeHintArgRefCallbackCall(int arg_num)
      : TypeHint(flag_contains_argref_inside),
        arg_num(arg_num) {}

public:
  int arg_num;

  static const TypeHint* create(int arg_num);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * instance<^1> — a rule indicating that i-th argument must be a constexpr string with a class name
 * For example, instance_cast($instance, $to_type) : instance<^2>
 */
class TypeHintArgRefInstance : public TypeHint {
  explicit TypeHintArgRefInstance(int arg_num)
      : TypeHint(flag_contains_argref_inside),
        arg_num(arg_num) {}

public:
  int arg_num;

  static const TypeHint* create(int arg_num);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * ...[*], like ^1[*] or ^2[*][*] — though it's used with argrefs only, it contains 'inner' to represent trees
 * Theoretically, later it could be not [*] (any_key) but [0] and others to get from tuples
 */
class TypeHintArgSubkeyGet : public TypeHint {
  explicit TypeHintArgSubkeyGet(const TypeHint* inner)
      : TypeHint(0),
        inner(inner) {}

public:
  const TypeHint* inner;

  static const TypeHint* create(const TypeHint* inner);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * T::fieldName — a reference to a type of a class field.
 * We allow this to be used only with generic T, see phpdoc_replace_genericTs_with_reified(); SomeClass::f will fail.
 */
class TypeHintRefToField : public TypeHint {
  explicit TypeHintRefToField(const TypeHint* inner, std::string field_name)
      : TypeHint(0),
        inner(inner),
        field_name(std::move(field_name)) {}

public:
  const TypeHint* inner;
  std::string field_name;

  static const TypeHint* create(const TypeHint* inner, const std::string& field_name);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;

  const TypeHint* resolve_field_type_hint() const;
};

/**
 * T::methodName() — a reference to a return type of a class instance method.
 * Like TypeHintRefToField, used in generics to express "a type that is returned from a method": `@ return T::getValue()`
 */
class TypeHintRefToMethod : public TypeHint {
  explicit TypeHintRefToMethod(const TypeHint* inner, std::string method_name)
      : TypeHint(0),
        inner(inner),
        method_name(std::move(method_name)) {}

public:
  const TypeHint* inner;
  std::string method_name;

  static const TypeHint* create(const TypeHint* inner, const std::string& method_name);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;

  FunctionPtr resolve_method() const;
};

/**
 * T[], like int[] or (?string)[]
 * Note that multidimensional arrays are trees: A[][] is TypeHintArray > TypeHintArray > TypeHintInstance
 * The 'array' keyword means "array of anything" and is any[] in face
 */
class TypeHintArray : public TypeHint {
  explicit TypeHintArray(const TypeHint* inner)
      : TypeHint(0),
        inner(inner) {}

public:
  const TypeHint* inner;

  static const TypeHint* create(const TypeHint* inner);
  static const TypeHint* create_array_of_any();

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * callable, callable(int):void
 * Remember, that untyped callables used as function arguments actually turn this function into a generic one
 */
class TypeHintCallable : public TypeHint {
  explicit TypeHintCallable()
      : TypeHint(flag_contains_callables_inside | flag_potentially_casts_rhs | flag_contains_autogeneric_inside | flag_contains_tp_any_inside) {}
  explicit TypeHintCallable(FunctionPtr f_bound_to)
      : TypeHint(flag_contains_callables_inside),
        f_bound_to(std::move(f_bound_to)) {}
  explicit TypeHintCallable(std::vector<const TypeHint*>&& arg_types, const TypeHint* return_type)
      : TypeHint(flag_contains_callables_inside | flag_potentially_casts_rhs),
        arg_types(arg_types),
        return_type(return_type) {}

  mutable InterfacePtr interface{nullptr}; // for typed callables; will be created on demand, see get_interface()

public:
  std::vector<const TypeHint*> arg_types; // for typed callables
  const TypeHint* return_type{nullptr};   // for typed callables
  FunctionPtr f_bound_to{nullptr};        // for untyped callables: not just 'callable', but bound to a function

  static const TypeHint* create(std::vector<const TypeHint*>&& arg_types, const TypeHint* return_type);
  static const TypeHint* create_untyped_callable();
  static const TypeHint* create_ptr_to_function(FunctionPtr f_bound_to);

  bool is_untyped_callable() const {
    return return_type == nullptr;
  }
  bool is_typed_callable() const {
    return return_type != nullptr;
  }

  InterfacePtr get_interface() const; // for typed callables
  ClassPtr get_lambda_class() const;  // for untyped callables: a class that wraps a bound lambda

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

// an arbitrary FFI type
// C type hints can come either from the C header file (FFI::cdef, FFI::load)
// or from ffi_cdata<...> type hint
//
// some examples of the types that can be expressed:
// ffi_cdata<scope, struct T>   = CData<struct T>
// ffi_cdata<scope, struct T*>  = CData<struct T*>
// ffi_cdata<scope, int64_t>    = CData<int64_t>
// ffi_cdata<scope, uint32_t[]> = CDataArray<uint32_t>
// ffi_cdata<scope, void*[][]>  = CDataArray<CDataArray<void*>>
class TypeHintFFIType : public TypeHint {
  TypeHintFFIType(std::string scope_name, const FFIType* type)
      : TypeHint(flag_contains_instances_inside),
        scope_name{std::move(scope_name)},
        type{type} {}

public:
  std::string scope_name;
  const FFIType* type;

  static const TypeHint* create(const std::string& scope_name, const FFIType* type, bool move = false);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const final;
};

/**
 * ffi_scope<^arg_num>
 */
class TypeHintFFIScopeArgRef : public TypeHint {
  explicit TypeHintFFIScopeArgRef(int arg_num)
      : TypeHint(flag_contains_argref_inside),
        arg_num{arg_num} {}

public:
  int arg_num;

  static const TypeHint* create(int arg_num);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const final;
};

/**
 * ffi_scope<scope_name>
 */
class TypeHintFFIScope : public TypeHint {
  explicit TypeHintFFIScope(std::string scope_name)
      : TypeHint(flag_contains_instances_inside),
        scope_name{std::move(scope_name)} {}

public:
  std::string scope_name;

  static const TypeHint* create(const std::string& scope_name);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const final;
};

/**
 * future<T>, future_queue<T>
 * They occur not only in functions.txt, but in phpdoc also of course
 */
class TypeHintFuture : public TypeHint {
  explicit TypeHintFuture(PrimitiveType ptype, const TypeHint* inner)
      : TypeHint(0),
        ptype(ptype),
        inner(inner) {}

public:
  PrimitiveType ptype;
  const TypeHint* inner;

  static const TypeHint* create(PrimitiveType ptype, const TypeHint* inner);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * not_null(T), not_false(T), not_null<T>, not_false<T>.
 * Primarily used in generics to unwrap genericT, similar to std::remove_reference.
 */
class TypeHintNotNull : public TypeHint {
  explicit TypeHintNotNull(const TypeHint* inner, bool drop_not_null, bool drop_not_false)
      : TypeHint(0),
        inner(inner),
        drop_not_null(drop_not_null),
        drop_not_false(drop_not_false) {}

public:
  const TypeHint* inner;
  bool drop_not_null;
  bool drop_not_false;

  static const TypeHint* create(const TypeHint* inner, bool drop_not_null, bool drop_not_false);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * A, VK\Namespaced, self — these all are instances
 * Note, that self/static/parent are saved as full_class_name, but storing them here is an intermediate moment of parsing
 * Because self is not constexpr: it should be replaced with an actual context to be used in inferring
 * See phpdoc_finalize_type_hint_and_resolve() and parse_classname() in phpdoc.cpp
 */
class TypeHintInstance : public TypeHint {
  explicit TypeHintInstance(std::string full_class_name, bool is_self_static_parent)
      : TypeHint(flag_contains_instances_inside | (is_self_static_parent ? flag_contains_self_static_parent_inside : 0)),
        full_class_name(std::move(full_class_name)) {}

  mutable ClassPtr klass;
  ClassPtr resolve_and_set_klass() const;

public:
  std::string full_class_name;

  static const TypeHint* create(const std::string& full_class_name);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;

  ClassPtr resolve() const {
    return klass ?: resolve_and_set_klass();
  }
};

/**
 * ?int, string|null, int|false, array|null|false — these are Optional
 * This class can be easily avoided, representing optional as |pipes, but it's more semantic
 */
class TypeHintOptional : public TypeHint {
  explicit TypeHintOptional(const TypeHint* inner, bool or_null, bool or_false)
      : TypeHint(0),
        inner(inner),
        or_null(or_null),
        or_false(or_false) {}

public:
  const TypeHint* inner;
  bool or_null{false};
  bool or_false{false};

  static const TypeHint* create(const TypeHint* inner, bool or_null, bool or_false);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * T1|T2|... — a pipe hint (a union hint as a synonym)
 * Note, that int|string is actually 'mixed' represented as TypeData
 * T|null|false are represented as TypeHintOptional (though leaving them as a pipe doesn't break anything)
 */
class TypeHintPipe : public TypeHint {
  explicit TypeHintPipe(std::vector<const TypeHint*>&& items)
      : TypeHint(0),
        items(items) {}

public:
  std::vector<const TypeHint*> items;

  static const TypeHint* create(std::vector<const TypeHint*>&& items);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * int, bool, false, any — all primitives without context, see enum PrimitiveType
 * Remember, that 'any' means 'automatically inferred', so when converted to TypeData, it's like absence of a hint
 */
class TypeHintPrimitive : public TypeHint {
  explicit TypeHintPrimitive(PrimitiveType ptype)
      : TypeHint(ptype == tp_any ? flag_contains_tp_any_inside : 0),
        ptype(ptype) {}

public:
  PrimitiveType ptype;

  static const TypeHint* create(PrimitiveType ptype);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * 'object' has a special treatment and a special type hint (it's not a primitive, it's not an instance of an exact class).
 * 'object' parameters, like just 'callable', convert a function to a template one.
 * 'object' also occurs in _functions.txt for parameters meaning "any instance, not a primitive".
 */
class TypeHintObject : public TypeHint {
  explicit TypeHintObject()
      : TypeHint(flag_contains_autogeneric_inside | flag_contains_tp_any_inside) {}

public:
  static const TypeHint* create();

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * shape(x:int, y:?string, ...) — associative arrays with predefined structure
 * While x and y exist as strings in TypeHint, they become Key (string keys) in TypeData subkeys
 */
class TypeHintShape : public TypeHint {
  explicit TypeHintShape(std::vector<std::pair<std::string, const TypeHint*>>&& items, bool is_vararg)
      : TypeHint(is_vararg ? flag_contains_tp_any_inside : 0),
        items(items),
        is_vararg(is_vararg) {}

  static std::map<std::int64_t, std::string> shape_keys_storage_;

public:
  std::vector<std::pair<std::string, const TypeHint*>> items;
  bool is_vararg{false};

  static const TypeHint* create(std::vector<std::pair<std::string, const TypeHint*>>&& items, bool is_vararg);

  static void register_known_key(std::int64_t key_hash, std::string_view key) noexcept;
  static const std::map<std::int64_t, std::string>& get_all_registered_keys() noexcept;

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;

  const TypeHint* find_at(const std::string& key) const;
};

/**
 * tuple(T1, T2) — numeric arrays with predefined items count
 * T_i can be any other type, even another tuple, it will me correctly parsed and mapped to TypeData
 */
class TypeHintTuple : public TypeHint {
  explicit TypeHintTuple(std::vector<const TypeHint*>&& items)
      : TypeHint(0),
        items(items) {}

public:
  std::vector<const TypeHint*> items;

  static const TypeHint* create(std::vector<const TypeHint*>&& items);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * T inside types of generics declarations, for example @ kphp-generic T1, T2
 * Then `T1` and `T2` in a context of that declaration (@ param, @ return, @ var) are of this class.
 */
class TypeHintGenericT : public TypeHint {
  explicit TypeHintGenericT(std::string nameT)
      : TypeHint(flag_contains_genericT_inside),
        nameT(std::move(nameT)) {}

public:
  std::string nameT;

  static const TypeHint* create(const std::string& nameT);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};

/**
 * class-string<T> — for generic functions (inner=T must be TypeHintGenericT)
 * Used to express parameters that are expected to be passed Some::class.
 */
class TypeHintClassString : public TypeHint {
  explicit TypeHintClassString(const TypeHint* inner)
      : TypeHint(0),
        inner(inner) {}

public:
  const TypeHint* inner;

  static const TypeHint* create(const TypeHint* inner);

  std::string as_human_readable() const final;
  void traverse(const TraverserCallbackT& callback) const final;
  const TypeHint* replace_self_static_parent(FunctionPtr resolve_context) const final;
  const TypeHint* replace_children_custom(const ReplacerCallbackT& callback) const final;
  void recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr func_call) const final;
};
