// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/convert-sprintf-calls.h"

#include "compiler/vertex-util.h"
#include <utility>

struct FormatCallInfo {
  FormatCallInfo()
      : args(VertexRange(Vertex::iterator{}, Vertex::iterator{})) {}
  // stores a variable that contains all the call arguments,
  // if all arguments are constant
  VertexAdaptor<op_var> var;
  // stores an array that contains all the call arguments,
  // if at least one argument is a function call or a variable
  VertexAdaptor<op_array> array;
  // flag for the presence of a value in format_args_var
  bool is_var{false};
  // stores a set of vertices of the call arguments from var or array
  VertexRange args;
};

struct FormatPart {
  enum class FormatSpecType { decimal, string };

  explicit FormatPart(std::string value)
      : value(std::move(value)) {}

  explicit FormatPart(FormatSpecType specifier)
      : specifier(specifier) {}

  std::string value;
  FormatSpecType specifier{};

  bool is_specifier() const {
    return value.empty();
  }
};

std::vector<FormatPart> try_parse_format_string(const std::string& format) {
  std::vector<FormatPart> parts;
  std::string last_value;
  bool find_percent = false;

  for (const auto& symbol : format) {
    if (find_percent) {
      if (symbol == 'd') {
        parts.emplace_back(FormatPart::FormatSpecType::decimal);
      } else if (symbol == 's') {
        parts.emplace_back(FormatPart::FormatSpecType::string);
      } else if (symbol == '%') {
        last_value += "%";
      } else {
        return {};
      }
      find_percent = false;
    } else if (symbol == '%') {
      find_percent = true;
      if (!last_value.empty()) {
        parts.emplace_back(std::move(last_value));
        last_value = "";
      }
    } else {
      last_value += symbol;
    }
  }

  if (!last_value.empty()) {
    parts.emplace_back(std::move(last_value));
  }

  return parts;
}

VertexPtr ConvertSprintfCallsPass::on_exit_vertex(VertexPtr root) {
  if (auto func_call = root.try_as<op_func_call>()) {
    const auto func = func_call->func_id;
    if (func->is_extern() && vk::any_of_equal(func->name, "sprintf", "vsprintf")) {
      return convert_sprintf_call(func_call);
    }
  }

  return root;
}

VertexPtr ConvertSprintfCallsPass::convert_sprintf_call(VertexAdaptor<op_func_call> call) {
  const auto args = call->args();
  const auto format_arg_raw = args[0];
  const auto* format_string = VertexUtil::get_constexpr_string(format_arg_raw);
  if (format_string == nullptr) {
    return call;
  }

  const auto parts = try_parse_format_string(*format_string);
  if (parts.empty()) {
    return call;
  }

  const auto count_specifiers = std::count_if(parts.begin(), parts.end(), [](const FormatPart& part) { return part.is_specifier(); });

  FormatCallInfo info;

  // if sprintf call with args
  if (args.size() > 1) {
    const auto format_args_raw = args[1];
    switch (format_args_raw->type()) {
    // if all arguments are constant
    case op_var: {
      info.var = format_args_raw.as<op_var>();
      info.array = info.var->var_id->init_val.try_as<op_array>();
      if (!info.array) {
        return call;
      }
      info.args = info.array->args();
      info.is_var = true;
      break;
    }
    // if at least one argument calls a function or variable
    case op_array: {
      info.array = format_args_raw.as<op_array>();
      info.args = info.array->args();
      break;
    }
    default:
      return call;
    }

    if (count_specifiers > info.args.size()) {
      return call;
    }
  }

  std::vector<VertexPtr> vertex_parts;
  size_t index_spec = 0;

  for (const auto& part : parts) {
    const auto vertex = convert_format_part_to_vertex(part, index_spec, info, call->location);
    vertex_parts.push_back(vertex);

    if (part.is_specifier()) {
      index_spec++;
    }
  }

  return VertexAdaptor<op_string_build>::create(vertex_parts).set_location(call->location);
}

VertexPtr ConvertSprintfCallsPass::convert_format_part_to_vertex(const FormatPart& part, size_t arg_index, const FormatCallInfo& info,
                                                                 const Location& call_location) {
  if (part.is_specifier()) {
    VertexPtr element;

    if (info.is_var) {
      // building $arr[$index]
      auto index_vertex = VertexAdaptor<op_int_const>::create();
      index_vertex->set_string(std::to_string(arg_index));
      element = VertexAdaptor<op_index>::create(info.var, index_vertex);
    } else {
      element = info.args[arg_index];
    }

    VertexPtr convert;

    switch (part.specifier) {
    case FormatPart::FormatSpecType::decimal:
      convert = VertexAdaptor<op_conv_int>::create(element);
      break;
    case FormatPart::FormatSpecType::string:
      convert = VertexAdaptor<op_conv_string>::create(element);
      break;
    default:
      return element;
    }

    return VertexAdaptor<op_conv_string>::create(convert);
  }

  VertexAdaptor<op_string> vertex = VertexAdaptor<op_string>::create().set_location(call_location);
  vertex->set_string(part.value);
  return vertex;
}
