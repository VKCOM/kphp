// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/generics-mixins.h"
#include "compiler/data/function-data.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/vertex.h"
#include "compiler/utils/string-utils.h"

/*
 * How do template functions work.
 * 
 * "template functions" are functions marked with @kphp-template, which expresses `f<T>` with phpdoc at declaration.
 * Unlike regular functions, they can be called with various instances: `f($a)`, `f($b)`, etc., implicitly meaning `f<A>($a)` and so on.
 * `f($primitive)` is proxied to `f<any>`.
 *
 * When `f<T>` is called like `f($obj)`, we detect T by an assumption (say, SomeClass) and create a function named "f$_$SomeClass".
 * `f($a)` makes T=A, `f($arr_or_users)` makes T=User[].
 * We can't express `f<User>($arr_of_users)` now, we don't have a syntax yet.
 *
 * Later, template functions will be deprecated in favor of generics support.
 * For now, we call them "template functions", but also use the word "generics" in some comments and sources.
 *
 * Say, we want to express `f<T1, T2>(T1 $arg0, T1 $arg1, int $arg2, T2 $arg3): T2[]`
 * 1) an old-fashioned syntax — still supported, but deprecated:
 *    @kphp-template $arg0 $arg1       // means they must be of the same classes
 *    @kphp-template T $arg3           // may be of another class; T to be used with @kphp-return
 *    @kphp-return T[]                 // for assumptions outerwise
 * 2) a new-way syntax — that looks more like generics:
 *    @kphp-template T1 T2
 *    @kphp-param T1 $arg0
 *    @kphp-param T1 $arg1
 *    @kphp-param T2 $arg3
 *    @kphp-return T2[]
 * For now, @kphp-param can be still only T1/T2 — not T1[] or other expressions, due to the reason described above.
 * Later, generics will support T1[], or tuple(T1,T2), or others — as T's will be able to be specified explicitly at a call.
 * 
 * T1 and T2 are represented as TypeHintGenericsT, to differ them from regular instances.
 * An instantiation (T=User[] for example) is performed by `phpdoc_replace_genericsT_with_instantiation()`.
 *
 * Functions with untyped `callable` arguments are auto-converted to template functions.
 * Later, we'll have a generics syntax to express `<T : SomeInterface>`, then SomeInterface would be `extends_hint` in terms of the code.
 * For now, extends_hint can only be an untyped callable, it's emerged from `f(callable $fn)`.
 * Now it's used to deduce, that `f('strlen')` is f<lambda_class>, not f<string>.
 *
 * call->instantiation_list is calculated in DeduceImplicitTypesAndCastsPass.
 * Functions are created in InstantiateGenericsAndLambdasPass.
 */


// an old syntax has only @kphp-template with $argument inside (or many such tags)
static void create_from_phpdoc_old_syntax(FunctionPtr f, GenericsDeclarationMixin *out, const std::vector<php_doc_tag> &phpdoc_tags) {
  for (const php_doc_tag &tag : phpdoc_tags) {
    switch (tag.type) {
      case php_doc_tag::kphp_template: {
        std::string nameT;    // can be before $arg
        const TypeHint *extends_hint = nullptr;
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          if (var_name[0] != '$') {
            kphp_error_return(nameT.empty(), "invalid @kphp-template syntax");
            nameT = static_cast<std::string>(var_name);
            continue;
          }
          if (nameT.empty()) {
            nameT = "T" + std::to_string(out->size() + 1);
          }

          auto param = f->find_param_by_name(var_name.substr(1));
          kphp_error_return(param, fmt_format("@kphp-template for {} — argument not found", var_name));
          extends_hint = param->type_hint && param->type_hint->try_as<TypeHintCallable>() ? param->type_hint : nullptr;
          param->type_hint = TypeHintGenericsT::create(nameT);
        }

        kphp_error_return(!nameT.empty(), "invalid @kphp-template syntax");
        out->add_itemT(nameT, extends_hint);
        break;
      }

      case php_doc_tag::kphp_param:
        kphp_error(0, "@kphp-param is not acceptable with old-style @kphp-template syntax");
        break;

      case php_doc_tag::kphp_return: {
        kphp_error_return(!out->empty(), "@kphp-template must precede @kphp-return");
        if (PhpDocTagParseResult doc_parsed = phpdoc_parse_type_and_var_name(tag.value, f)) {
          f->return_typehint = doc_parsed.type_hint;    // typically, T or T::field
        }
        break;
      }

      default:
        break;
    }
  }

  kphp_assert(!f->generics_declaration->empty());
}

// a new syntax is @kphp-template T1 T2 with many @kphp-param tags
static void create_from_phpdoc_new_syntax(FunctionPtr f, GenericsDeclarationMixin *out, const std::vector<php_doc_tag> &phpdoc_tags) {
  for (const php_doc_tag &tag : phpdoc_tags) {
    switch (tag.type) {
      case php_doc_tag::kphp_template:
        for (auto s : split_skipping_delimeters(tag.value, " ")) {
          out->add_itemT(static_cast<std::string>(s), nullptr);
        }
        break;

      case php_doc_tag::kphp_param: {
        kphp_error_return(!out->empty(), "@kphp-template must precede @kphp-param");
        if (PhpDocTagParseResult doc_parsed = phpdoc_parse_type_and_var_name(tag.value, f)) {
          auto param = f->find_param_by_name(doc_parsed.var_name);
          kphp_error_return(param, fmt_format("@kphp-param for unexisting argument ${}", doc_parsed.var_name));
          param->type_hint = doc_parsed.type_hint;
        }
        break;
      }

      case php_doc_tag::kphp_return:
        kphp_error_return(!out->empty(), "@kphp-template must precede @kphp-return");
        if (PhpDocTagParseResult doc_parsed = phpdoc_parse_type_and_var_name(tag.value, f)) {
          f->return_typehint = doc_parsed.type_hint;    // typically, T or T::field
        }
        break;

      default:
        break;
    }
  }

  kphp_assert(!f->generics_declaration->empty());
}


bool GenericsDeclarationMixin::has_nameT(const std::string &nameT) const {
  for (const GenericsItem &item : itemsT) {
    if (item.nameT == nameT) {
      return true;
    }
  }
  return false;
}

void GenericsDeclarationMixin::add_itemT(const std::string &nameT, const TypeHint *extends_hint) {
  itemsT.emplace_back(GenericsItem{nameT, extends_hint});
}

const TypeHint *GenericsDeclarationMixin::find(const std::string &nameT) const {
  for (const GenericsItem &item : itemsT) {
    if (item.nameT == nameT) {
      return item.extends_hint;
    }
  }
  return nullptr;
}


void GenericsInstantiationMixin::add_instantiationT(const std::string &nameT, const TypeHint *instantiation) {
  auto insertion_result = instantiations.emplace(nameT, instantiation);
  if (!insertion_result.second) {
    const TypeHint *previous_inst = insertion_result.first->second;
    kphp_error(previous_inst == instantiation,
               fmt_format("generics <{}> is both {} and {}", nameT, previous_inst->as_human_readable(), instantiation->as_human_readable()));
  }
}

const TypeHint *GenericsInstantiationMixin::find(const std::string &nameT) const {
  auto it = instantiations.find(nameT);
  return it == instantiations.end() ? nullptr : it->second;
}

std::string GenericsInstantiationMixin::generate_instantiated_func_name(FunctionPtr template_function) const {
  // an instantiated function name will be "{original_name}$_${postfix}", where postfix = "T1$T2"
  std::string name = template_function->name + "$_";
  for (const auto &name_and_type : instantiations) {
    name += "$";
    name += replace_non_alphanum(name_and_type.second->as_human_readable());
  }
  return name;
}


void GenericsDeclarationMixin::apply_from_phpdoc(FunctionPtr f, vk::string_view phpdoc_str) {
  if (!f->generics_declaration) {
    f->generics_declaration = new GenericsDeclarationMixin();
  }

  const std::vector<php_doc_tag> &phpdoc_tags = parse_php_doc(phpdoc_str);

  bool is_new_syntax = std::any_of(phpdoc_tags.begin(), phpdoc_tags.end(),
                                   [](const php_doc_tag &tag) { return tag.type == php_doc_tag::kphp_param; });
  if (is_new_syntax) {
    create_from_phpdoc_new_syntax(f, f->generics_declaration, phpdoc_tags);
  } else {
    create_from_phpdoc_old_syntax(f, f->generics_declaration, phpdoc_tags);
  }
}

void GenericsDeclarationMixin::make_function_generics_on_callable_arg(FunctionPtr f, VertexPtr func_param) {
  if (!f->generics_declaration) {
    f->generics_declaration = new GenericsDeclarationMixin();
  }

  std::string nameT = "Callback" + std::to_string(f->generics_declaration->size() + 1);
  func_param.as<op_func_param>()->type_hint = TypeHintGenericsT::create(nameT);

  // add a <Cb1: callable> rule; the presence of 'callable' is important: imagine
  // function f(callable $cb) { ... }
  // f('some_fn');
  // even if f() is called with a string, it should be instantiated as f<callable$xxx>, not f<any> or f<string>
  const TypeHint *extends_hint_callable = TypeHintCallable::create_untyped_callable();
  f->generics_declaration->add_itemT(nameT, extends_hint_callable);
}
