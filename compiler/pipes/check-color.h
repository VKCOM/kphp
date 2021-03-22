// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>

#include "compiler/function-colors.h"
#include "compiler/function-pass.h"
#include "compiler/compiler-core.h"
#include "common/termformat/termformat.h"
#include "common/algorithms/contains.h"

class CheckColorPass final : public FunctionPassBase {
  using Stacktrace = std::vector<FunctionPtr>;

private:
  PaletteTree tree;

public:
  CheckColorPass() {
    tree = G->get_function_palette_tree();
  }

  void check() const {
    Stacktrace stacktrace{};
    this->check_step(stacktrace, this->tree.root(), this->current_function);
  }

  void on_start() final {
    this->check();
  }

  void on_finish() override {}

  std::string get_description() override {
    return "Check function color";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }

private:
  // check_func_caller is a function that checks functions above the given in callstack.
  void check_func_caller(const Stacktrace& stacktrace, PaletteNode* rule, FunctionPtr func) const {
    // If the current rule describes an error.
    if (rule->is_leaf && rule->is_error) {
      // If the rule has no children, then we immediately cause an error.
      if (rule->children.empty()) {
        error(stacktrace, rule->message);
        return;
      }

      // However, if there are children, but the current function
      // is not called anywhere, then we also cause an error.
      if (func->dep_rev.empty()) {
        error(stacktrace, rule->message);
        return;
      }

      // If the rule has children and the current function is called
      // somewhere, there is no need to cause an error, since the
      // calling function will probably allow the use and there will be no error.
    }

    for (auto& call_in_func : func->dep_rev) {
      const auto contains = vk::contains(stacktrace, call_in_func);
      if (contains) {
        continue;
      }

      this->check_step(stacktrace, rule, call_in_func);
    }
  }

  void check_step(Stacktrace stacktrace, PaletteNode* rule, FunctionPtr func) const {
    stacktrace.push_back(func);

    // The rules are structured in such a way that if there is a rule
    // starting with any, then it must be processed in any case, regardless
    // of the colors of the current function.
    //
    // In order to handle complex cases when a rule with any has clarifying rules,
    // at the stage of rule initialization, any is converted to any_except,
    // which is processed separately.
    auto* any_rule = rule->match(Color::any);
    if (any_rule != nullptr) {
      this->check_func_caller(stacktrace, any_rule, func);
    }

    // If the current rules have a child with any_except color, in addition,
    // we need to get it for further processing.
    auto* any_except_rule = rule->match(Color::any_except);

    // If a function has no color, then it is considered transparent.
    if (func->colors.empty()) {
      if (any_except_rule != nullptr) {
        // Rules starting with any_except can be skipped if more precise
        // rules were found in the process of processing the colors of the
        // function.
        //
        // However, if the function does not have colors, then there are no
        // colors that can cause any_except rules to be skipped, so they are
        // executed anyway.
        this->check_func_caller(stacktrace, any_except_rule, func);
      }

      // Also, due to the fact that there are no colors and the function is
      // transparent, we call the processing of the functions above the
      // current in callstack.
      this->check_func_caller(stacktrace, rule, func);
      return;
    }

    // A variable storing the fact that the rule did not match any color of the current
    // function. This means that we need to call the processing of functions above the
    // current in callstack.
    // This variable helps to do this only once, so that there are no multiple identical
    // errors.
    auto all_color_not_match = true;

    // Variable containing the shift in the colors of the current function.
    // This variable shows the number of colors already processed.
    auto shift_in_colors = 0;

    // used_colors contains all the colors that will be used to process the current
    // function. This is necessary to cancel matching any_except rules if the colors
    // used are rules that are more precise than any_except.
    auto used_colors = PaletteNodeContainer();

    // We process the colors until all of them have been processed in one way or another.
    while (shift_in_colors < func->colors.size()) {

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

      if (count_used_colors == 1) {
        used_colors.add(tmp_rule);
      }

      // If at least one color is matched, then set the variable to false.
      all_color_not_match = false;

      // Increase the shift in colors by the number of used colors so as not
      // to process already processed colors.
      shift_in_colors += count_used_colors;

      // Call the check of the functions above the callstack
      // with the previously obtained rule.
      this->check_func_caller(stacktrace, tmp_rule, func);
    }

    if (any_except_rule != nullptr) {
      // Since there can be branching rules for any, each branch has its own list
      // of colors that specify the rules for any_except.
      //
      // We need to check all of them and for those for which there will be no matches
      // with used_colors, call the check, so we will call only the necessary any_except
      // rules and skip those that do not need to be processed, since a rule that
      // is more precise than any_except has already been processed earlier.
      for (auto& pair : any_except_rule->except_for_color) {
        const auto except_colors = pair.second;

        auto has = false;

        for (int i = 0; i < PaletteNodeContainer::Size; ++i) {
          const auto except_has_color = except_colors.has(Color(i));
          const auto used_has_color = used_colors.has(Color(i));

          if (except_has_color && used_has_color) {
            has = true;
            break;
          }
        }

        if (!has) {
          // Since we save a pointer to the beginning of the rule, we need to create
          // a fake parent so that in check_step, this parent has the received rule
          // and is applied.
          auto tmp_node = PaletteNode(Color::none);
          tmp_node.children.add(pair.first);
          this->check_func_caller(stacktrace, &tmp_node, func);
        }
      }
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
      this->check_func_caller(stacktrace, rule, func);
    }
  }

  static PaletteNode* try_use_max_colors(const FunctionPtr& func, int shift_in_colors, PaletteNode* rule, int& count_used_colors) {
    // We copy the pointer to the current rule so as not to modify the original.
    auto* tmp_rule = rule;

    // The starting index is the last index minus the color shift.
    // Thus, each iteration of the while loop (in the calling function)
    // we will bypass different colors.
    const auto start_index = func->colors.size() - 1 - shift_in_colors;

    // The number of colors matched in a row.
    count_used_colors = 0;

    // We go around the colors from the end.
    for (int i = start_index; i >= 0; --i) {
      const auto color = func->colors[i];

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
    stage::set_function(stacktrace.back());

    const auto child = stacktrace.front();
    const auto parent = stacktrace.back();

    const auto parent_name = parent->get_human_readable_name() + "()";
    const auto child_name = child->get_human_readable_name() + "()";

    std::string desc;

    desc += format_stacktrace(stacktrace);

    kphp_error(0, fmt_format("{} ({} call {})\n\n{}",
                             message,
                             TermStringFormat::paint_green(parent_name),
                             TermStringFormat::paint_green(child_name),
                             desc)
    );
  }

  static string format_stacktrace(const Stacktrace &stacktrace) {
    std::string desc;

    for (int i = stacktrace.size() - 1; i >= 0; --i) {
      const auto frame = stacktrace[i];
      const auto name = frame->get_human_readable_name() + "()";

      desc += "  ";
      desc += TermStringFormat::paint(name, TermStringFormat::yellow);

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
};
