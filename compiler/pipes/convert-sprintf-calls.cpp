// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/convert-sprintf-calls.h"

struct FormatCallInfo {
  FormatCallInfo() : args(VertexRange(Vertex::iterator{}, Vertex::iterator{})) {}
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
  enum class FormatSpecType {
    Decimal,
    String
  };

  std::string value;
  FormatSpecType specifier;

  bool is_specifier() const {
    return value.empty();
  }
};

std::vector<FormatPart> try_parse_format_string(const std::string &format) {
  std::vector<FormatPart> parts;
  std::string last_value;
  bool find_percent = false;

  for (const auto &symbol : format) {
    if (find_percent) {
      if (symbol == 'd') {
        parts.push_back(FormatPart{"", FormatPart::FormatSpecType::Decimal});
      } else if (symbol == 's') {
        parts.push_back(FormatPart{"", FormatPart::FormatSpecType::String});
      } else if (symbol == '%') {
        last_value += "%";
      } else {
        return {};
      }
      find_percent = false;
    } else if (symbol == '%') {
      find_percent = true;
      if (!last_value.empty()) {
        parts.push_back(FormatPart{last_value});
        last_value = "";
      }
    } else {
      last_value += symbol;
    }
  }

  if (!last_value.empty()) {
    parts.push_back(FormatPart{last_value});
  }

  return parts;
}

VertexPtr ConvertSprintfCallsPass::on_exit_vertex(VertexPtr root) {
  if (auto func_call = root.try_as<op_func_call>()) {
    const auto func = func_call->func_id;
    if (vk::any_of_equal(func->name, "sprintf", "vsprintf")) {
      return convert_sprintf_call(func_call);
    }
  }

  return root;
}

VertexPtr ConvertSprintfCallsPass::convert_sprintf_call(VertexAdaptor<op_func_call> &call) {
  const auto args = call->args();
  auto format_arg_raw = args[0];

  if (format_arg_raw->type() == op_conv_string) {
    format_arg_raw = format_arg_raw.as<op_conv_string>()->expr();
  }

  VertexAdaptor<op_string> format_string;

  switch (format_arg_raw->type()) {
    case op_var: {
      const auto format_var = format_arg_raw.as<op_var>()->var_id;
      if (!format_var) {
        return call;
      }
      const auto format_string_val = format_var->init_val.try_as<op_string>();
      if (!format_string_val) {
        return call;
      }
      format_string = format_string_val;
      break;
    }
    case op_string: {
      format_string = format_arg_raw.as<op_string>();
      break;
    }
    default:
      return call;
  }

  const auto parts = try_parse_format_string(format_string->str_val);
  if (parts.empty()) {
    return call;
  }
  const auto count_specifiers = std::count_if(parts.begin(), parts.end(), [](const FormatPart &part) {
    return part.is_specifier();
  });

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

  for (const auto &part : parts) {
    const auto vertex = convert_format_part_to_vertex(part, index_spec, info);
    vertex_parts.push_back(vertex);

    if (part.is_specifier()) {
      index_spec++;
    }
  }

  return VertexAdaptor<op_string_build>::create(vertex_parts);
}

VertexPtr ConvertSprintfCallsPass::convert_format_part_to_vertex(const FormatPart &part, size_t arg_index, const FormatCallInfo &info) {
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
      case FormatPart::FormatSpecType::Decimal:
        convert = VertexAdaptor<op_conv_int>::create(element);
        break;
      case FormatPart::FormatSpecType::String:
        convert = VertexAdaptor<op_conv_string>::create(element);
        break;
      default:
        return element;
    }

    return VertexAdaptor<op_conv_string>::create(convert);
  }

  VertexAdaptor<op_string> vertex = VertexAdaptor<op_string>::create();
  vertex->set_string(part.value);
  return vertex;
}
