// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/generics-mixins.h"

#include "compiler/data/function-data.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"

/*
 * Generic functions are functions marked with @kphp-generic, which expresses `f<T>` with phpdoc at declaration.
 * Parameters can depend on T: `f<T>(T $arg)`, `f<T>(T[] $arr)`, `f<T1,T2>(tuple(T1,T2) $a)`.
 * In phpdoc, just @param/@return are used, KPHP treats T there not as TypeHintInstance, but as TypeHintGenericT:
 *  / **
 *    * @kphp-generic T1
 *    * @param T1[] $arr
 *    * @return T1[]
 *    * /
 *   function filterGreater(array $arr, int $level) { ... }
 *
 * When `f<T>(T $arg)` is called, we reify T by an assumption: `f($a)` makes T=A, `f($arr_or_users)` makes T=User[].
 * When `f<T>(T[] $arr)` is called like `f($arr_of_users)`, then T=User, and `f($single_user)` is an error.
 * See generics-reification.cpp.
 *
 * Besides auto-reification, where is a PHP-code syntax to manually provide instantiation types:
 *    f/ *<T1, T2>* /(...)
 *
 * In @kphp-generic, one can use ':' syntax to provide extends_hint:
 *    @kphp-generic T: SomeInterface
 * Functions with untyped `callable` or `object` arguments are auto-converted to generic functions,
 * meaning `T: callable` and `T: object` correspondingly.
 * Particularly, `T: callable` is used to deduce, that `f('strlen')` is f<lambda_class>, not f<string>.
 *
 * Besides extends_hint, there is '=' syntax to provide def_hint:
 *    @kphp-generic T1, T2 = mixed, T3 = T2
 * It's especially useful with generic classes:
 *    @kphp-generic T, TCont = Container<T>
 *    @kphp-generic TValue, TErr : \Err = \Err
 *
 * There are also variadic generics, declared like
 *    @kphp-generic ...TArg
 * They are applicable only to variadic functions, e.g.
 *    function f<...TArg>(TArg ...$args) { anotherF(...$args); }
 * When called like `f(1, new A)`, N=2, a separate function `f$n2` is created,
 * replacing "...TArg" with "TArg1, TArg2" and "...$args" with "$args$1, $args$2":
 *    function f$n2<TArg1, TArg2>(TArg1 $args$1, TArg2 $args$2) { anotherF($args$1, $args$2); }
 * Consider `convert_variadic_generic_function_accepting_N_args()`.
 *
 * Note, that @kphp-generic is parsed right in gentree, even before parsing function/class,
 * because even when parsing function body we must know that nested / *<T>* / is generic T, not class T.
 * In other words, we treat <T> as a language syntax, which is expressed in phpdoc only due to PHP limitations.
 *
 * call->reifiedTs is calculated in DeduceImplicitTypesAndCastsPass.
 * Functions are created in InstantiateGenericsAndLambdasPass.
 * An instantiation (T=User[] for example) is performed by `phpdoc_replace_genericTs_with_reified()`.
 * For `f<T>` with T=SomeClass, an instantiated function is named "f$_$SomeClass".
 */

// an old syntax has only @kphp-template with $argument inside (or many such tags)
static void create_from_phpdoc_kphp_template_old_syntax(FunctionPtr f, GenericsDeclarationMixin *out, const PhpDocComment *phpdoc) {
  for (const PhpDocTag &tag : phpdoc->tags) {
    switch (tag.type) {
      case PhpDocType::kphp_template: {
        std::string nameT; // can be before $arg
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
          param->type_hint = TypeHintGenericT::create(nameT);
        }

        kphp_error_return(!nameT.empty(), "invalid @kphp-template syntax");
        out->add_itemT(nameT, extends_hint, nullptr);
        break;
      }

      case PhpDocType::kphp_param:
        kphp_error(0, "@kphp-param is not acceptable with old-style @kphp-template syntax");
        break;

      case PhpDocType::kphp_return: {
        kphp_error_return(!out->empty(), "@kphp-template must precede @kphp-return");
        if (auto tag_parsed = tag.value_as_type_and_var_name(f, out)) {
          f->return_typehint = tag_parsed.type_hint; // typically, T or T::field
        }
        break;
      }

      default:
        break;
    }
  }

  kphp_assert(!out->empty());
}

// a new syntax is `@kphp-generic T1, T2`
// * every T may have extends_hint: "T: callable" / "T: BaseClass"
// * every T may have def_hint: "T = mixed", "T = \VK\Err"
// (if both, it looks like "T: Err = Err")
// (@param tags depending on T's are parsed afterward, like regular @param: T will be interpreted as TypeHintGenericT)
static void create_from_phpdoc_kphp_generic_new_syntax(FunctionPtr current_function, GenericsDeclarationMixin *out, const PhpDocComment *phpdoc) {
  for (const PhpDocTag &tag : phpdoc->tags) {
    switch (tag.type) {
      case PhpDocType::kphp_generic:
        for (auto s : split_skipping_delimeters(tag.value, ",")) {
          std::vector<Token> tokens = phpdoc_to_tokens(s);
          auto cur_tok = tokens.cbegin();
          bool is_variadic = false;

          if (cur_tok->type() == tok_varg) { // not T, but ...T (variadic generic)
            is_variadic = true;
            cur_tok++;
          }

          kphp_error_return(cur_tok->type() != tok_var_name, fmt_format("'@kphp-generic ${}' is an invalid syntax: provide T names, not variables.\nProbably, "
                                                                        "you want:\n* @kphp-generic T\n* @param T ${}",
                                                                        cur_tok->str_val, cur_tok->str_val));
          kphp_error_return(cur_tok->type() == tok_func_name, fmt_format("Can't parse generic declaration: can't detect T from '{}'", s));

          std::string nameT = static_cast<std::string>(cur_tok->str_val);
          const TypeHint *extends_hint = nullptr;
          const TypeHint *def_hint = nullptr;

          cur_tok++;
          if (cur_tok->type() == tok_colon) {
            PhpDocTypeHintParser parser(current_function, out);
            extends_hint = parser.parse_from_tokens_silent(++cur_tok);
            kphp_error_return(extends_hint, fmt_format("Can't parse generic declaration <{}> after ':'", nameT));
            // since this is called from gentree, we don't have classes to resolve, so check_declarationT_extends_hint() will be called later
            // also, for each call `f<T>()`, we'll check that particular T, see check_reifiedT_extends_hint()
          }
          if (cur_tok->type() == tok_eq1) {
            PhpDocTypeHintParser parser(current_function, out);
            def_hint = parser.parse_from_tokens_silent(++cur_tok);
            kphp_error_return(def_hint, fmt_format("Can't parse generic declaration <{}> after '='", nameT));
            kphp_error_return(cur_tok->type() != tok_colon, fmt_format("Can't parse generic declaration <{}>: ':' should go before a default value", nameT));
          }
          kphp_error_return(cur_tok->type() != tok_func_name,
                            "Can't parse generic declaration: use a comma to separate generic T ('T1, T2' instead of 'T1 T2')");
          kphp_error_return(cur_tok->type() == tok_end, fmt_format("Can't parse generic declaration <{}>: unexpected end", nameT));

          out->add_itemT(nameT, extends_hint, def_hint, is_variadic);
        }
        break;

      case PhpDocType::kphp_param:
        kphp_error_return(0, "Don't use @kphp-param with generics, use just @param");
        break;

      case PhpDocType::kphp_return:
        kphp_error_return(0, "Don't use @kphp-return with generics, use just @return");
        break;

      default:
        break;
    }
  }

  kphp_assert(!out->empty());
}

std::string GenericsDeclarationMixin::as_human_readable() const {
  return "<"
         + vk::join(itemsT, ", ",
                    [](const GenericsItem &itemT) {
                      if (itemT.is_variadic) {
                        return "..." + itemT.nameT;
                      }
                      return itemT.nameT;
                    })
         + ">";
}

bool GenericsDeclarationMixin::has_nameT(const std::string &nameT) const {
  for (const GenericsItem &item : itemsT) {
    if (item.nameT == nameT) {
      return true;
    }
  }
  return false;
}

void GenericsDeclarationMixin::add_itemT(const std::string &nameT, const TypeHint *extends_hint, const TypeHint *def_hint, bool is_variadic) {
  kphp_error_return(!nameT.empty(), "Invalid (empty) generic <T> in declaration");
  kphp_error_return(!has_nameT(nameT), fmt_format("Duplicate generic <{}> in declaration", nameT));
  kphp_error_return(itemsT.empty() || !itemsT.back().is_variadic, fmt_format("Variadic <...{}> is not the last in declaration", itemsT.back().nameT));
  itemsT.emplace_back(GenericsItem{nameT, extends_hint, def_hint, is_variadic});
}

const TypeHint *GenericsDeclarationMixin::get_extends_hint(const std::string &nameT) const {
  for (const GenericsItem &item : itemsT) {
    if (item.nameT == nameT) {
      return item.extends_hint;
    }
  }
  return nullptr;
}

std::string GenericsDeclarationMixin::prompt_provide_commentTs_human_readable(VertexPtr call) const {
  std::string call_str = replace_call_string_to_readable(call->get_string());
  std::string params_str = call->size() > (call->extra_type == op_ex_func_call_arrow ? 1 : 0) ? "(...)" : "()";
  std::string t_str = vk::join(itemsT, ", ", [](const GenericsItem &itemT) { return itemT.nameT; });

  return "Please, provide all generic types using syntax " + call_str + "/*<" + t_str + ">*/" + params_str;
}

std::string GenericsInstantiationPhpComment::as_human_readable() const {
  return "/*<" + vk::join(vectorTs, ", ", [](const TypeHint *instantiationT) { return instantiationT->as_human_readable(); }) + ">*/";
}

GenericsInstantiationMixin::GenericsInstantiationMixin(const GenericsInstantiationMixin &rhs) {
  if (rhs.commentTs != nullptr) {
    this->commentTs = new GenericsInstantiationPhpComment(*rhs.commentTs);
  }
}

std::string GenericsInstantiationMixin::as_human_readable() const {
  if (!instantiations.empty()) {
    return vk::join(instantiations, ", ", [](const auto &pair) { return pair.first + "=" + pair.second->as_human_readable(); });
  }

  if (commentTs != nullptr) {
    return commentTs->as_human_readable();
  }
  return "empty";
}

void GenericsInstantiationMixin::provideT(const std::string &nameT, const TypeHint *instantiationT, VertexPtr call) {
  auto insertion_result = instantiations.emplace(nameT, instantiationT);
  if (!insertion_result.second) {
    const TypeHint *previous_inst = insertion_result.first->second;
    FunctionPtr generic_function = call.as<op_func_call>()->func_id;
    kphp_error(previous_inst == instantiationT,
               fmt_format("Couldn't reify generic <{}> for call: it's both {} and {}.\n{}", nameT, previous_inst->as_human_readable(),
                          instantiationT->as_human_readable(), generic_function->genericTs->prompt_provide_commentTs_human_readable(call)));
  }
}

const TypeHint *GenericsInstantiationMixin::find(const std::string &nameT) const {
  auto it = instantiations.find(nameT);
  return it == instantiations.end() ? nullptr : it->second;
}

std::string GenericsInstantiationMixin::generate_instantiated_name(const std::string &orig_name, const GenericsDeclarationMixin *genericTs) const {
  // an instantiated function name will be "{orig_name}$_${postfix}", where postfix = "T1$_$T2"
  std::string name = orig_name;
  for (const auto &itemT : *genericTs) {
    name += "$_$";
    name += replace_non_alphanum(instantiations.find(itemT.nameT)->second->as_human_readable());
  }
  return name;
}

// having parsed and resolved 'T: SomeInterface' in @kphp-generic, check that extends_hint
void GenericsDeclarationMixin::check_declarationT_extends_hint(const TypeHint *extends_hint, const std::string &nameT) {
  // we allow only 'T: SomeClass' / 'T: SomeInterface' and 'T: callable' created implicitly
  // if an unknown class is specified, an error was printed before (on resolving)
  if (!extends_hint || extends_hint->try_as<TypeHintCallable>() || // 'T: callable' is created implicitly for untyped 'callable' params
      extends_hint->try_as<TypeHintObject>() ||                    // 'T: object'
      extends_hint->try_as<TypeHintInstance>() ||                  // 'T: BaseClass' / 'T: SomeInterface'
      extends_hint->try_as<TypeHintPipe>()) {                      // 'T: int|string' / 'T: SomeInterface|SomeInterface2'
    return;
  }

  kphp_error_return(!extends_hint->try_as<TypeHintOptional>(), "A generic T can't extend ?Some or Some|false");
  kphp_error_return(!extends_hint->try_as<TypeHintPrimitive>(), "A generic T can't extend a primitive, what does it mean?");
  kphp_error_return(!extends_hint->try_as<TypeHintArray>(), "A generic T can't extend an array; use just T in declaration, and T[] in @param");
  kphp_error_return(0, fmt_format("Strange or unsupported extends condition in generic T after '{}:'", nameT));
}

// having parsed and resolved 'T = Err' in @kphp-generic, check that def_hint
void GenericsDeclarationMixin::check_declarationT_def_hint(const TypeHint *def_hint, const std::string &nameT) {
  // we allow anything as a default
  // if it's 'self', it was already resolved up to this point
  // we also allow 'T2 = T1' (T1 is TypeHintGenericT)
  if (!def_hint || !def_hint->has_argref_inside()) {
    return;
  }

  kphp_error_return(0, fmt_format("Strange or unsupported default type in generic T after '{}='", nameT));
}

bool GenericsDeclarationMixin::is_new_kphp_generic_syntax(const PhpDocComment *phpdoc) {
  // @kphp-generic is a new syntax, @kphp-template is an old one
  return phpdoc && phpdoc->find_tag(PhpDocType::kphp_generic);
}

// creates genericTs as an empty object, ready for parsing
GenericsDeclarationMixin *GenericsDeclarationMixin::create_for_function_empty(FunctionPtr f) {
  auto *out = new GenericsDeclarationMixin();
  (void)f; // reserved for the future
  return out;
}

// creates genericTs parsed from @kphp-generic above the function
GenericsDeclarationMixin *GenericsDeclarationMixin::create_for_function_from_phpdoc(FunctionPtr f, const PhpDocComment *phpdoc) {
  auto *out = create_for_function_empty(f);

  if (is_new_kphp_generic_syntax(phpdoc)) {
    create_from_phpdoc_kphp_generic_new_syntax(f, out, phpdoc);
  } else {
    create_from_phpdoc_kphp_template_old_syntax(f, out, phpdoc);
  }

  return out;
}

// creates genericTs as a clone from generic_f->genericTs, replacing "...TArgs" with "TArg1, TArg2" (for N=2 here)
GenericsDeclarationMixin *GenericsDeclarationMixin::create_for_function_cloning_from_variadic(FunctionPtr generic_f, int n_variadic) {
  auto *out = create_for_function_empty(generic_f);

  for (const auto &itemT : *generic_f->genericTs) {
    if (itemT.is_variadic) {
      for (int i = 1; i <= n_variadic; ++i) {
        auto ith_nameT = itemT.nameT + std::to_string(i);
        out->add_itemT(ith_nameT, itemT.extends_hint, nullptr);
      }
    } else {
      out->add_itemT(itemT.nameT, itemT.extends_hint, itemT.def_hint);
    }
  }

  return out;
}

void GenericsDeclarationMixin::make_function_generic_on_callable_arg(FunctionPtr f, VertexPtr func_param) {
  if (!f->genericTs) {
    f->genericTs = GenericsDeclarationMixin::create_for_function_empty(f);
  }

  std::string nameT = "Callback" + std::to_string(f->genericTs->size() + 1);
  func_param.as<op_func_param>()->type_hint = TypeHintGenericT::create(nameT);

  // add a <Callback1: callable> rule; the presence of 'callable' is important: imagine
  // function f(callable $cb) { ... }
  // f('some_fn');
  // even if f() is called with a string, it should be instantiated as f<lambda$xxx>, not f<string>
  const TypeHint *extends_hint_callable = TypeHintCallable::create_untyped_callable();
  f->genericTs->add_itemT(nameT, extends_hint_callable, nullptr);
}

void GenericsDeclarationMixin::make_function_generic_on_object_arg(FunctionPtr f, VertexPtr func_param) {
  if (!f->genericTs) {
    f->genericTs = GenericsDeclarationMixin::create_for_function_empty(f);
  }

  std::string nameT = "Object" + std::to_string(f->genericTs->size() + 1);
  func_param.as<op_func_param>()->type_hint = TypeHintGenericT::create(nameT);

  // add a <Object1: object> rule
  f->genericTs->add_itemT(nameT, TypeHintObject::create(), nullptr);
}
