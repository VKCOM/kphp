// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "common/wrappers/string_view.h"

namespace vk {

namespace tlo_parsing {
struct tl_scheme;
}

namespace tl {

enum class php_field_type { t_string, t_int, t_mixed, t_double, t_bool, t_bool_true, t_array, t_maybe, t_class, t_unknown };

bool is_or_null_possible(php_field_type internal_type);

struct PhpClassField {
  std::string field_name;
  std::string php_doc_type;
  std::string field_mask_name;
  int field_mask_bit;
  php_field_type field_value_type;
  bool use_other_type;
};

struct PhpClassRepresentation {
  std::string tl_name;
  std::string php_class_namespace;
  std::string php_class_name;
  const PhpClassRepresentation* parent{nullptr};
  bool is_interface{false};
  bool is_builtin{false};
  std::vector<PhpClassField> class_fields;

  std::string get_full_php_class_name() const;
};

struct TlFunctionPhpRepresentation {
  std::unique_ptr<const PhpClassRepresentation> function_args;
  std::unique_ptr<const PhpClassRepresentation> function_result;
  bool is_kphp_rpc_server_function{false};
};

struct TlTypePhpRepresentation {
  std::unique_ptr<const PhpClassRepresentation> type_representation;
  std::vector<std::unique_ptr<const PhpClassRepresentation>> constructors;
};

bool is_php_code_gen_allowed(const TlTypePhpRepresentation& type_repr) noexcept;
bool is_php_code_gen_allowed(const TlFunctionPhpRepresentation& func_repr) noexcept;

struct PhpClasses {
  void load_from(const vk::tlo_parsing::tl_scheme& scheme, bool generate_tl_internals);

  std::unordered_map<std::string, TlFunctionPhpRepresentation> functions;
  std::unordered_map<std::string, TlTypePhpRepresentation> types;
  std::unordered_map<std::string, std::reference_wrapper<const PhpClassRepresentation>> all_classes;
  std::unordered_multimap<int, std::reference_wrapper<const PhpClassRepresentation>> magic_to_classes;

  static const char* tl_parent_namespace() {
    return "VK";
  }
  static const char* tl_namespace() {
    return "TL";
  }
  static const char* tl_full_namespace() {
    return R"(VK\TL)";
  }

  static const char* types_namespace() {
    return "Types";
  }
  static const char* functions_namespace() {
    return "Functions";
  }
  static const char* common_engine_namespace() {
    return "_common";
  }

  static const char* rpc_response_type() {
    return "RpcResponse";
  }
  static const char* rpc_response_type_with_tl_namespace() {
    return R"(TL\RpcResponse)";
  }

  static const char* rpc_response_ok() {
    return "rpcResponseOk";
  }
  static const char* rpc_response_ok_with_tl_namespace() {
    return R"(TL\_common\Types\rpcResponseOk)";
  }
  static const char* rpc_response_ok_with_tl_full_namespace() {
    return R"(VK\TL\_common\Types\rpcResponseOk)";
  }

  static const char* rpc_response_header() {
    return "rpcResponseHeader";
  }
  static const char* rpc_response_header_with_tl_namespace() {
    return R"(TL\_common\Types\rpcResponseHeader)";
  }
  static const char* rpc_response_header_with_tl_full_namespace() {
    return R"(VK\TL\_common\Types\rpcResponseHeader)";
  }

  static const char* rpc_response_error() {
    return "rpcResponseError";
  }
  static const char* rpc_response_error_with_tl_namespace() {
    return R"(TL\_common\Types\rpcResponseError)";
  }
  static const char* rpc_response_error_with_tl_full_namespace() {
    return R"(VK\TL\_common\Types\rpcResponseError)";
  }

  static const char* rpc_function() {
    return "RpcFunction";
  }

  static const char* rpc_function_return_result() {
    return "RpcFunctionReturnResult";
  }
  static const char* rpc_function_return_result_with_tl_namespace() {
    return R"(TL\RpcFunctionReturnResult)";
  }
};

} // namespace tl
} // namespace vk
