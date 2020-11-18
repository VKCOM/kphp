#include <gtest/gtest.h>

#include "compiler/lexer.h"
#include "compiler/debug.h"

TEST(lexer_test, test_php_tokens) {
  struct testCase {
    std::string input;
    std::vector<std::string> expected;
  };
  std::vector<testCase> tests = {
    {"exit", {"tok_func_name(exit)"}},
    {"exit()", {"tok_func_name(exit)", "tok_oppar(()", "tok_clpar())"}},
    {"die", {"tok_func_name(die)"}},
    {"die()", {"tok_func_name(die)", "tok_oppar(()", "tok_clpar())"}},

    {";", {"tok_semicolon(;)"}},

    {"'abc'", {"tok_str(abc)"}},
    {"12 + 4", {"tok_int_const(12)", "tok_plus(+)", "tok_int_const(4)"}},
    {"$x['a']", {"tok_var_name($x)", "tok_opbrk([)", "tok_str(a)", "tok_clbrk(])"}},
    {"$x[-40]", {"tok_var_name($x)", "tok_opbrk([)", "tok_minus(-)", "tok_int_const(40)", "tok_clbrk(])"}},
    {"$x->y->z", {"tok_var_name($x)", "tok_arrow(->)", "tok_func_name(y)", "tok_arrow(->)", "tok_func_name(z)"}},

    // strings: non-interpolated forms
    {"'$x $y->z'", {"tok_str($x $y->z)"}},
    {"'$x[0] $x[key]'", {"tok_str($x[0] $x[key])"}},

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
