#include <gtest/gtest.h>

#include "compiler/lexer.h"
#include "compiler/debug.h"

TEST(lexer_test, test_php_tokens) {
  std::string debugTokenName(TokenType t);  // implemented in debug.cpp

  struct testCase {
    std::string input;
    std::vector<std::string> expected;
  };
  std::vector<testCase> tests = {
    {"exit", {"tok_func_name(exit)"}},
    {"exit()", {"tok_func_name(exit)", "tok_oppar(()", "tok_clpar())"}},
    {"die", {"tok_func_name(die)"}},
    {"die()", {"tok_func_name(die)", "tok_oppar(()", "tok_clpar())"}},

    {"$obj->exit", {"tok_var_name($obj)", "tok_arrow(->)", "tok_func_name(exit)"}},
    {"$obj->exit()", {"tok_var_name($obj)", "tok_arrow(->)", "tok_func_name(exit)", "tok_oppar(()", "tok_clpar())"}},
    {"$obj->throw", {"tok_var_name($obj)", "tok_arrow(->)", "tok_func_name(throw)"}},
    {"Example::for", {"tok_func_name(Example::for)"}},
    {"Example::for()", {"tok_func_name(Example::for)", "tok_oppar(()", "tok_clpar())"}},

    // elseif is turned into else+if pair
    {"elseif", {"tok_else", "tok_if"}},
    {"} elseif", {"tok_clbrc(})", "tok_else", "tok_if"}},

    // elseif is not replaced with else+if if it preceded by `const` or `function`
    {"function elseif()", {"tok_function(function)", "tok_elseif(elseif)", "tok_oppar(()", "tok_clpar())"}},
    {"public function elseif()", {"tok_public(public)", "tok_function(function)", "tok_elseif(elseif)", "tok_oppar(()", "tok_clpar())"}},
    {"private static function elseif", {"tok_private(private)", "tok_static(static)", "tok_function(function)", "tok_elseif(elseif)"}},
    {"const elseif", {"tok_const(const)", "tok_elseif(elseif)"}},
    {"const true", {"tok_const(const)", "tok_true(true)"}},

    {";", {"tok_semicolon(;)"}},

    {"} finally {", {"tok_clbrc(})", "tok_finally(finally)", "tok_opbrc({)"}},
    {"function finally()", {"tok_function(function)", "tok_finally(finally)", "tok_oppar(()", "tok_clpar())"}},
    {"public function finally()", {"tok_public(public)", "tok_function(function)", "tok_finally(finally)", "tok_oppar(()", "tok_clpar())"}},
    {"private static function finally", {"tok_private(private)", "tok_static(static)", "tok_function(function)", "tok_finally(finally)"}},
    {"const finally", {"tok_const(const)", "tok_finally(finally)"}},

    {"new Exception()", {"tok_new(new)", "tok_func_name(Exception)", "tok_oppar(()", "tok_clpar())"}},
    {"new \\Exception()", {"tok_new(new)", "tok_func_name(\\Exception)", "tok_oppar(()", "tok_clpar())"}},
    {"new Exception('test')", {"tok_new(new)", "tok_func_name(Exception)", "tok_oppar(()", "tok_str(test)", "tok_clpar())"}},

    {"'abc'", {"tok_str(abc)"}},
    {"12 + 4", {"tok_int_const(12)", "tok_plus(+)", "tok_int_const(4)"}},
    {"12_100 + 4_5.56", {"tok_int_const_sep(12_100)", "tok_plus(+)", "tok_float_const_sep(4_5.56)"}},
    {"$x['a']", {"tok_var_name($x)", "tok_opbrk([)", "tok_str(a)", "tok_clbrk(])"}},
    {"$x[-40]", {"tok_var_name($x)", "tok_opbrk([)", "tok_minus(-)", "tok_int_const(40)", "tok_clbrk(])"}},
    {"$x->y->z", {"tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_arrow(->)", "tok_func_name(z)"}},
    {"fn() => 1", {"tok_fn(fn)", "tok_oppar(()", "tok_clpar())", "tok_double_arrow(=>)", "tok_int_const(1)"}},

    // check that "fn" keyword is lexed correctly inside class member context
    {"$fn", {"tok_var_name($fn)"}},
    {"$x->fn", {"tok_var_name($x)", "tok_arrow(->)", "tok_func_name(fn)"}},
    {"C::fn", {"tok_func_name(C::fn)"}},
    {"C::$fn", {"tok_var_name(C::fn)"}},
    {"C::fn()", {"tok_func_name(C::fn)", "tok_oppar(()", "tok_clpar())"}},

    // strings: non-interpolated forms
    {"'$x $y->z'", {"tok_str($x $y->z)"}},
    {"'$x[0] $x[key]'", {"tok_str($x[0] $x[key])"}},

    // 'float' and 'double' are different tokens, unless it's a cast
    {"float", {"tok_float(float)"}},
    {"double", {"tok_double(double)"}},
    {"(float)$x", {"tok_conv_float", "tok_var_name($x)"}},
    {"(double)$x", {"tok_conv_float", "tok_var_name($x)"}},

    // strings: mix variables with simple tok_str parts
    {R"("$x")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_str_end(\")"}},
    {R"(".$x")", {"tok_str_begin(\")", "tok_str(.)", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_str_end(\")"}},
    {R"("$x.")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_str(.)", "tok_str_end(\")"}},
    {R"(".$x.")", {"tok_str_begin(\")", "tok_str(.)", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_str(.)", "tok_str_end(\")"}},
    {R"("$x$y")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_expr_begin", "tok_var_name($y)", "tok_expr_end", "tok_str_end(\")"}},
    {R"("$x $y")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_str( )", "tok_expr_begin", "tok_var_name($y)", "tok_expr_end", "tok_str_end(\")"}},

    // strings: $x->y syntax
    {R"("$x->y")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_expr_end", "tok_str_end(\")"}},
    {R"("{$x->y}")", {"tok_str_begin(\")", "tok_expr_begin({)", "tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_expr_end(})", "tok_str_end(\")"}},

    // strings: ${var} syntax
    // note that debug_str for vars is "${x}", but str_val is just "x"
    {R"("${x}")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name(${x})", "tok_expr_end", "tok_str_end(\")"}},
    {R"("a${foo}b")", {"tok_str_begin(\")", "tok_str(a)", "tok_expr_begin", "tok_var_name(${foo})", "tok_expr_end", "tok_str(b)", "tok_str_end(\")"}},

    // strings: $x[int] syntax
    {R"("$x[0]")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_opbrk([)", "tok_int_const(0)", "tok_clbrk(])", "tok_expr_end", "tok_str_end(\")"}},
    {R"("{$x[0]}")", {"tok_str_begin(\")", "tok_expr_begin({)", "tok_var_name($x)", "tok_opbrk([)", "tok_int_const(0)", "tok_clbrk(])", "tok_expr_end(})", "tok_str_end(\")"}},

    // strings: $x[-int] syntax; negative int keys are supported since PHP 7.1
    {R"("$x[-5]")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_opbrk([)", "tok_minus(-)", "tok_int_const(5)", "tok_clbrk(])", "tok_expr_end", "tok_str_end(\")"}},
    {R"("{$x[-5]}")", {"tok_str_begin(\")", "tok_expr_begin({)", "tok_var_name($x)", "tok_opbrk([)", "tok_minus(-)", "tok_int_const(5)", "tok_clbrk(])", "tok_expr_end(})", "tok_str_end(\")"}},

    // strings: try to confuse the lexer with \\ that should not be part of a name
    {"\"$x\\n\"", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_str(\n)", "tok_str_end(\")"}},
    {"\"$x\n\"", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_expr_end", "tok_str(\n)", "tok_str_end(\")"}},
    {"\"$x->y\\n\"", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_expr_end", "tok_str(\n)", "tok_str_end(\")"}},
    {"\"$x->y\n\"", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_expr_end", "tok_str(\n)", "tok_str_end(\")"}},

    // strings: identifiers inside simple variable expressions produce tok_str.
    {R"("$x[key]")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_opbrk([)", "tok_str(key)", "tok_clbrk(])", "tok_expr_end", "tok_str_end(\")"}},
    {R"("[key]")", {"tok_str([key])"}},

    // strings: simple syntax doesn't support indexing more than 1 dim at once
    {R"("$x[key][notkey]")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_opbrk([)", "tok_str(key)", "tok_clbrk(])", "tok_expr_end", "tok_str([notkey])", "tok_str_end(\")"}},
    {R"("$x[0][0][0]")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_opbrk([)", "tok_int_const(0)", "tok_clbrk(])", "tok_expr_end", "tok_str([0][0])", "tok_str_end(\")"}},

    // strings: simple syntax doesn't support accessing more than 1 instance member at once
    {R"("$x->y->z")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_expr_end", "tok_str(->z)", "tok_str_end(\")"}},

    // strings: simple syntax doesn't support indexing of the accessed instance member
    {R"("$x->y[0]")", {"tok_str_begin(\")", "tok_expr_begin", "tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_expr_end", "tok_str([0])", "tok_str_end(\")"}},

    {"'a'[0]", {"tok_str(a)", "tok_opbrk([)", "tok_int_const(0)", "tok_clbrk(])"}},
    {"[$a][0]", {"tok_opbrk([)", "tok_var_name($a)", "tok_clbrk(])", "tok_opbrk([)", "tok_int_const(0)", "tok_clbrk(])"}},
    {"$a[0]", {"tok_var_name($a)", "tok_opbrk([)", "tok_int_const(0)", "tok_clbrk(])"}},
    {"('a' . 'b')[0]", {"tok_oppar(()", "tok_str(a)", "tok_dot(.)", "tok_str(b)", "tok_clpar())", "tok_opbrk([)", "tok_int_const(0)", "tok_clbrk(])"}},

    {"[1]", {"tok_opbrk([)", "tok_int_const(1)", "tok_clbrk(])"}},
    {"array(1)", {"tok_array(array)", "tok_oppar(()", "tok_int_const(1)", "tok_clpar())"}},

    {"define", {"tok_define(define)"}},
    {"\\define", {"tok_define(\\define)"}},
    {"defined", {"tok_defined(defined)"}},
    {"\\defined", {"tok_defined(\\defined)"}},

    // combined tests
    {"echo \"{$x->y}\";", {"tok_echo(echo)", "tok_str_begin(\")", "tok_expr_begin({)", "tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_expr_end(})", "tok_str_end(\")", "tok_semicolon(;)"}},
  };

  for (const auto &test : tests) {
    std::string input = "<?php\n" + test.input;
    auto tokens = php_text_to_tokens(input);

    std::vector<std::string> expected = test.expected;
    expected.emplace_back("tok_semicolon");
    expected.emplace_back("tok_end");

    std::vector<std::string> actual;
    std::transform(tokens.begin(), tokens.end(), std::back_inserter(actual),
      [](Token tok) -> std::string {
        auto tok_str = debugTokenName(tok.type());
        if (!tok.debug_str.empty()) {
          return tok_str + "(" + static_cast<std::string>(tok.debug_str) + ")";
        }
        if (!tok.str_val.empty()) {
          return tok_str + "(" + static_cast<std::string>(tok.str_val) + ")";
        }
        return tok_str;
      });

    ASSERT_EQ(expected, actual) << "input was: " << test.input;
  }
}

TEST(lexer_test, test_heredoc_string_skip_spaces) {
  struct testCase {
    std::string input;
    std::string expected;
  };
  std::vector<testCase> tests = {
    // empty strings or spaces
    {"<<<END\nEND;", ""},
    {"<<<END\n END;", ""},
    {"<<<END\n  END;", ""},
    {"<<<END\n\nEND;", ""},
    {"<<<END\n \nEND;", " "},
    {"<<<END\n  \nEND;", "  "},
    {"<<<END\n  \n END;", " "},
    {"<<<END\n  \n  END;", ""},
    {"<<<END\n\n\nEND;", "\n"},
    // no indent
    {"<<<END\na\nEND;", "a"},
    {"<<<END\n a\nEND;", " a"},
    {"<<<END\n  a\nEND;", "  a"},
    {"<<<END\n  a\n\nEND;", "  a\n"},
    // with space indent
    {"<<<END\n a\n END;", "a"},
    {"<<<END\n  a\n END;", " a"},
    {"<<<END\n  a\n  END;", "a"},
    {"<<<END\n  a\n\n END;", " a\n"},
    {"<<<END\n  a\n\n  END;", "a\n"},
    // with tab indent
    {"<<<END\n\ta\n\tEND;", "a"},
    {"<<<END\n\t\ta\n\tEND;", "\ta"},
    {"<<<END\n\t\ta\n\t\tEND;", "a"},
    {"<<<END\n\t\ta\n\n\tEND;", "\ta\n"},
    {"<<<END\n\t\ta\n\n\t\tEND;", "a\n"},
    // nowdoc
    {"<<<'END'\n a\n END;", "a"},
    {"<<<'END'\n  a\n END;", " a"},
    {"<<<'END'\n  a\n  END;", "a"},
    {"<<<'END'\n  a\n\n END;", " a\n"},
    {"<<<'END'\n  a\n\n  END;", "a\n"},
    // few lanes
    {"<<<END\n a\n b\nEND;", " a\n b"},
    {"<<<END\n a\n b\n END;", "a\nb"},
    {"<<<END\n  a\n  b\nEND;", "  a\n  b"},
    {"<<<END\n  a\n  b\n END;", " a\n b"},
    {"<<<END\n  a\n  b\n  END;", "a\nb"},
    // ending whitespaces
    {"<<<END\n  a\n \nEND;", "  a\n "},
    {"<<<END\n  a\n  \nEND;", "  a\n  "},
    {"<<<END\n  a\n  \n\nEND;", "  a\n  \n"},
    {"<<<END\n  a\n  \n \nEND;", "  a\n  \n "},
    {"<<<END\n  a\n \n END;", " a\n"},
    {"<<<END\n  a\n  \n END;", " a\n "},
    {"<<<END\n  a\n  \n\n END;", " a\n \n"},
    {"<<<END\n  a\n  \n \n END;", " a\n \n"},
    {"<<<END\n  a\n\n \n END;", " a\n\n"},
    {"<<<END\n   a\n \n   END;", "a\n"},
    {"<<<END\n  a\n \n  END;", "a\n"},
    {"<<<END\n  a\n  \n  END;", "a\n"},
    {"<<<END\n  a\n  \n\n  END;", "a\n\n"},
    {"<<<END\n  a\n  \n \n  END;", "a\n\n"},
    {"<<<END\n  a\n  \n  \n  END;", "a\n\n"},
    {"<<<END\n  a\n  \n  \n  END;", "a\n\n"},
  };

  for (const auto &[input, expected] : tests) {
    auto input_tag = "<?php\n" + input;
    auto tokens = php_text_to_tokens(input_tag);
    ASSERT_TRUE(!tokens.empty());
    auto str_token = tokens.front();
    ASSERT_TRUE(str_token.type() == tok_str);
    ASSERT_EQ(str_token.str_val, expected) << "input was: " << input;
  }
}
