// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-colors.h"
#include "compiler/function-pass.h"
#include <common/termformat/termformat.h>

#include <utility>

#include "compiler/compiler-core.h"

class CheckColorPass final : public FunctionPassBase {
  using Stacktrace = std::vector<FunctionPtr>;

private:
  PaletteTree tree;

public:
  CheckColorPass() {
    tree = G->get_function_palette_tree();
  }

  void check() {
    Stacktrace stacktrace{};
    this->check_step(stacktrace, this->tree.root(), this->current_function);
  }

  // check_func is a function that checks functions above the given in callstack.
  void check_func(const Stacktrace& stacktrace, PaletteNode* rule, FunctionPtr func) {
    // If the current rule has a subtree starting with the color 'any',
    // then first of all it is necessary to check if this rule matches.
    auto* any_rule = rule->match(Color::any);
    if (any_rule != nullptr) {
      this->check_func(stacktrace, any_rule, func);
    }

    // If the current rule describes an error.
    if (rule->is_leaf && rule->is_error) {
      // If the rule has no children, then we immediately cause an error.
      if (rule->children.empty()) {
        error(stacktrace, rule->message);
        return;
      } else {
        // However, if there are children, but the current function
        // is not called anywhere, then we also cause an error.
        if (func->called_in.empty()) {
          error(stacktrace, rule->message);
          return;
        }
      }
      // If the rule has children and the current function is called
      // somewhere, there is no need to cause an error, since the
      // calling function will probably allow the use and there will be no error.
    }

    for (auto& call_in_func : func->called_in) {
      this->check_step(stacktrace, rule, call_in_func);
    }
  }

  void check_step(Stacktrace stacktrace, PaletteNode * rule, FunctionPtr func) {
    stacktrace.push_back(func);

    // If a function has no color, then it is considered transparent.
    if (func->colors.empty()) {
      // Therefore, we call the method to check the functions above the current in callstack.
      this->check_func(stacktrace, rule, func);
      return;
    }

    // A variable storing the fact that the rule did not match any color of the current function.
    // This means that we need to call the processing of functions above the current in callstack.
    // This variable helps to do this only once, so that there are no multiple identical errors.
    auto all_color_not_match = true;

    // Variable containing the shift in the colors of the current function.
    // This variable shows the number of colors already processed.
    auto shift_in_colors = 0;

    // We process the colors until all of them have been processed in one way or another.
    while (shift_in_colors < func->colors.length()) {

      // Trying to match the largest number of colors in a row, color by color.
      //
      // That is, if there is a rule:
      //   'red blue green'
      //     and
      //   function @green and @blue,
      //
      // then we first try to match the green color, which will be found and
      // the 'red blue' rule will return, and then the blue one, thus obtaining
      // the remainder of the 'red' rule, which will be the rule that will need
      // to be matched further.
      //
      // count_used_colors stores the amount that was able to be matched in a row at a time.
      auto count_used_colors = 0;
      auto* tmp_rule = try_use_max_colors(func, shift_in_colors, rule, count_used_colors);

      // If count_used_colors is zero, which means that it was not possible to find suitable
      // colors in this iteration, then go to the next iteration.
      if (count_used_colors == 0) {
        shift_in_colors++;
        continue;
      }

      // If at least one color is matched, then set the variable to false.
      all_color_not_match = false;

      // Increase the shift in colors by the number of used colors so as not
      // to process already processed colors.
      shift_in_colors += count_used_colors;

      // Call the check of the functions above the callstack with the previously obtained rule.
      this->check_func(stacktrace, tmp_rule, func);
    }

    // If all colors have not been matched for the current function,
    // we need to call the check for the functions above on the callstack.
    //
    // This can happen if, for example, we have the rule:
    //    'highload no-highload'
    //   and callstack:
    //    highload          ssr         no-highload
    //       f1()    ->     f2()    ->     f3()
    //
    // Here, starting with f3(), we will first match the rule and expect
    // the highload color for the functions higher in the callstack.
    // When we go to f2(), there will not be any colors that match the rule
    // that we are currently matching, which means that we need to check the
    // functions higher in the callstack and already in the f1() function
    // we will match the rule.
    if (all_color_not_match) {
      this->check_func(stacktrace, rule, func);
    }
  }

  static PaletteNode* try_use_max_colors(const FunctionPtr& func, int shift_in_colors, PaletteNode* rule, int& count_used_colors) {
    // We copy the pointer to the current rule so as not to modify the original.
    auto* tmp_rule = rule;

    // The starting index is the last index minus the color shift.
    // Thus, each iteration of the while loop (in the calling function)
    // we will bypass different colors.
    const auto start_index = func->colors.length() - 1 - shift_in_colors;

    // The number of colors matched in a row.
    count_used_colors = 0;

    // We go around the colors from the end.
    for (int i = start_index; i >= 0; --i) {
      const auto color = func->colors.colors()[i];

      // Trying to match the current color to the current rule.
      auto* temp_rule_node = tmp_rule->match(color);
      if (temp_rule_node == nullptr) {
        // If the match failed, it means that the current iteration has failed.
        break;
      }

      count_used_colors++;
      tmp_rule = temp_rule_node;
    }

    return tmp_rule;
  }

  static void error(const Stacktrace& stacktrace, const std::string& message) {
    auto *file = stdout;
    const auto with_color = stage::should_be_colored(file);
    stage::set_function(stacktrace.back());

    const auto child = stacktrace[0];
    const auto parent = stacktrace[stacktrace.size() - 1];

    const auto parent_name = parent->get_human_readable_name() + "()";
    const auto child_name = child->get_human_readable_name() + "()";

    std::string desc;

    desc += format_stacktrace(stacktrace, with_color);

    kphp_error(0, fmt_format("{} ({} call {})\n\n{}",
                             message,
                             with_color ? TermStringFormat::paint_green(parent_name) : parent_name,
                             with_color ? TermStringFormat::paint_green(child_name) : child_name,
                             desc)
    );
  }

  static string format_stacktrace(const Stacktrace &stacktrace, bool with_color) {
    std::string desc;

    for (int i = stacktrace.size() - 1; i >= 0; --i) {
      const auto frame = stacktrace[i];
      const auto name = frame->get_human_readable_name() + "()";

      desc += "  ";
      desc += with_color ? TermStringFormat::paint(name, TermStringFormat::yellow) : name;

      if (!frame->colors.empty()) {
        desc += " with following ";
        desc += frame->colors.to_string();
      }

      if (i != 0) {
        desc += "\n\n";
      }
    }

    return desc;
  }

  void on_start() final {
    Stacktrace stacktrace{};
    this->check();
  }

  void on_finish() override {}

  std::string get_description() override {
    return "Check function color";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }
};
