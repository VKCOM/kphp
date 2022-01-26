// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl2php/gen-php-tests.h"

#include <fstream>
#include <iterator>
#include <unordered_map>
#include <vector>

#include "common/tl2php/gen-php-common.h"
#include "common/tl2php/php-classes.h"
#include "common/wrappers/string_view.h"

namespace vk {
namespace tl {
namespace {

struct TypesAndFunctions {
  std::vector<std::reference_wrapper<const PhpClassRepresentation>> types;
  std::vector<std::reference_wrapper<const PhpClassRepresentation>> functions;
  std::vector<std::reference_wrapper<const TlFunctionPhpRepresentation>> kphp_server_functions;
};

struct PhpPolyfills {
  friend std::ostream &operator<<(std::ostream &os, const PhpPolyfills &) {
    return os <<
              "#ifndef KPHP" << std::endl
              << "require_once '"
              << PhpClasses::tl_parent_namespace() << '/'
              << PhpClasses::tl_namespace() << '/'
              << PhpClasses::rpc_function() << ".php';" << R"php(

spl_autoload_register(function ($class) {
  $filename = trim(str_replace('\\','/', $class), '/') . '.php';
  require_once $filename;
});

function warning($m) {}

function to_array_debug($a) {
  return (array)$a;
}
#endif

/**
 * @kphp-inline
 * @return RpcConnection
 */
function _new_rpc_connection(string $host, int $port) {
#ifndef KPHP
  return 1;
#endif
  return new_rpc_connection($host, $port);
}

/**
 * @kphp-inline
 * @param RpcConnection $connection
 */
function _typed_rpc_tl_query_one($connection, \VK\TL\RpcFunction $rpc_function): int {
#ifndef KPHP
  return 1;
#endif
  return typed_rpc_tl_query_one($connection, $rpc_function);
}

/**
 * @kphp-inline
 * @param RpcConnection $connection
 */
function _typed_rpc_tl_query($connection, array $rpc_functions): array {
#ifndef KPHP
  return array_fill(0, count($rpc_functions), 1);
#endif
  return typed_rpc_tl_query($connection, $rpc_functions);
}

/**
 * @kphp-inline
 * @return \VK\TL\RpcResponse
 */
function _typed_rpc_tl_query_result_one(int $query_id) {
#ifndef KPHP
  return new TL\_common\Types\rpcResponseError(-4000, 'Failed to establish connection [probably reconnect timeout is not expired]');
#endif
  return typed_rpc_tl_query_result_one($query_id);
}

/**
 * @kphp-inline
 */
function _typed_rpc_tl_query_result(array $query_ids): array {
#ifndef KPHP
  return array_fill(0, count($query_ids), _typed_rpc_tl_query_result_one(1));
#endif
  return typed_rpc_tl_query_result($query_ids);
}

/**
 * @kphp-inline
 */
function _typed_rpc_tl_query_result_synchronously(array $query_ids): array {
#ifndef KPHP
  return _typed_rpc_tl_query_result($query_ids);
#endif
  return typed_rpc_tl_query_result_synchronously($query_ids);
}

/**
 * @kphp-inline
 * @return \VK\TL\RpcFunction
 */
function _rpc_server_fetch_request() {
#ifndef KPHP
  throw new Exception('Not enough data to fetch');
#endif
  return rpc_server_fetch_request();
}

/**
 * @kphp-inline
 * @param \VK\TL\RpcFunctionReturnResult $response
 */
function _rpc_server_store_response($response) {
#ifndef KPHP
  return;
#endif
  rpc_server_store_response($response);
}

)php" <<
              "$CONSTRUCTORS = [" << std::endl
              << "  " << PhpClasses::rpc_response_ok_with_tl_namespace() << "::class," << std::endl
              << "  " << PhpClasses::rpc_response_header_with_tl_namespace() << "::class," << std::endl
              << "  " << PhpClasses::rpc_response_error_with_tl_namespace() << "::class" << std::endl
              << "];" << SkipLine{};
  }
};

struct CreateInstanceAndDump {
  const PhpClassRepresentation &repr;

  friend std::ostream &operator<<(std::ostream &os, const CreateInstanceAndDump &self) {
    auto instance_var_name = self.repr.php_class_name + "_instance";
    os << std::endl
       << "  $" << instance_var_name << " = new " << ClassNameWithNamespace{self.repr} << ";" << std::endl
       << "  var_dump(to_array_debug($" << instance_var_name << "));" << std::endl;

    if (self.repr.parent && self.repr.parent->php_class_name == "RpcFunction") {
      os << "  var_dump($" << instance_var_name << "->getTLFunctionName());" << std::endl
         << "  $functions[] = $" << instance_var_name << ";" << std::endl
         << "  $query = _typed_rpc_tl_query_one($stub_connection, $" << instance_var_name << ");" << std::endl
         << "  $result = _typed_rpc_tl_query_result_one($query);" << std::endl
         << "  var_dump(get_class($result));" << std::endl
         << "  " << ClassNameWithNamespace{self.repr} << "::result($result);" << std::endl;
    }
    return os;
  }
};

struct TestRpcTlQueryResult {
  const char *function_name;

  friend std::ostream &operator<<(std::ostream &os, const TestRpcTlQueryResult &self) {
    return os
      << "/**" << std::endl
      << " * @kphp-inline" << std::endl
      << " * @param RpcConnection $stub_connection" << std::endl
      << " */" << std::endl
      << "function test" << self.function_name << "($stub_connection, array $functions) {" << std::endl
      << "  $queries = _typed_rpc_tl_query($stub_connection, $functions);" << std::endl
      << "  $results = " << self.function_name << "($queries);" << std::endl
      << "  foreach($results as $result) {" << std::endl
      << "    var_dump(get_class($result));" << std::endl
      << "  }" << std::endl
      << "}" << SkipLine{};
  }
};

void gen_tests_for_classes(const std::string &test_file_path,
                           const TypesAndFunctions &classes) {
  std::ofstream os{test_file_path};
  os << "@ok" << std::endl
     << PhpTag{}
     << "use \\" << PhpClasses::tl_full_namespace() << ";" << SkipLine{}
     << PhpPolyfills{};

  if (!classes.types.empty()) {
    os << "function test_types() {";
    for (const auto &klass : classes.types) {
      os << CreateInstanceAndDump{klass.get()};
    }
    os << "}" << SkipLine{}
       << "test_types();" << SkipLine{};
  }

  if (!classes.functions.empty()) {
    os << TestRpcTlQueryResult{"_typed_rpc_tl_query_result"}
       << TestRpcTlQueryResult{"_typed_rpc_tl_query_result_synchronously"}
       << "function test_functions() {" << std::endl
       << "  $stub_connection = _new_rpc_connection('0.0.0.0', 0);" << std::endl
       << "  $functions = [];" << std::endl;
    for (const auto &klass : classes.functions) {
      os << CreateInstanceAndDump{klass.get()};
    }
    os << SkipLine{}
       << "  test_typed_rpc_tl_query_result($stub_connection, $functions);" << std::endl
       << "  test_typed_rpc_tl_query_result_synchronously($stub_connection, $functions);" << std::endl
       << "}"
       << SkipLine{}
       << "test_functions();" << SkipLine{};
  }

  if (!classes.kphp_server_functions.empty()) {
    os << "function test_kphp_server_functions() {" << std::endl
       << "  try {" << std::endl
       << "    $request = _rpc_server_fetch_request();" << std::endl
       << "    var_dump(get_class($request));" << std::endl
       << "  } catch(Exception $e) {" << std::endl
       << "    var_dump($e->getMessage());" << std::endl
       << "  }" << SkipLine{};
    for (const auto &function : classes.kphp_server_functions) {
      assert(function.get().function_result->class_fields.size() == 1);
      os << "  $response = " << ClassNameWithNamespace{*function.get().function_args}
         << "::createRpcServerResponse(" << DefaultValue{function.get().function_result->class_fields.back()} << ");" << std::endl
         << "  _rpc_server_store_response($response);" << SkipLine{};
    }
    os << "}" << SkipLine{}
       << "test_kphp_server_functions();" << SkipLine{};
  }
}

std::string make_test_file_name(const std::string &dir, const vk::string_view &ns) {
  std::string test_file_name{"test_"};
  std::transform(ns.begin(), ns.end(),
                 std::back_inserter(test_file_name),
                 [](char c) {
                   return c == '\\' ? '_' : std::tolower(c);
                 });
  if (test_file_name.back() == '_') {
    test_file_name.append(PhpClasses::common_engine_namespace());
  }
  test_file_name.insert(0, dir).append(".php");
  return test_file_name;
}

vk::string_view get_tl_namespace(const PhpClassRepresentation &repr) {
  const size_t dot = repr.tl_name.find('.');
  return dot == std::string::npos ? vk::string_view{} : vk::string_view{repr.tl_name}.substr(0, dot);
}

} // namespace

void gen_php_tests(const std::string &dir, const PhpClasses &classes) {
  std::unordered_map<vk::string_view, TypesAndFunctions> classes_by_namespaces;
  for (const auto &type_repr : classes.types) {
    if (!is_php_code_gen_allowed(type_repr.second)) {
      continue;
    }

    if (!type_repr.second.type_representation->is_interface) {
      classes_by_namespaces[get_tl_namespace(*type_repr.second.type_representation)].types.emplace_back(*type_repr.second.type_representation);
    }
    for (const auto &klass : type_repr.second.constructors) {
      classes_by_namespaces[get_tl_namespace(*klass)].types.emplace_back(*klass);
    }
  }

  for (const auto &function_repr : classes.functions) {
    if (!is_php_code_gen_allowed(function_repr.second)) {
      continue;
    }

    if (!function_repr.second.function_args->is_interface) {
      auto &classes = classes_by_namespaces[get_tl_namespace(*function_repr.second.function_args)];
      classes.functions.emplace_back(*function_repr.second.function_args);
      if (function_repr.second.is_kphp_rpc_server_function) {
        classes.kphp_server_functions.emplace_back(function_repr.second);
      }
    }
  }

  for (const auto &ns_and_klass : classes_by_namespaces) {
    std::string test_file_name = make_test_file_name(dir, ns_and_klass.first);
    gen_tests_for_classes(test_file_name, ns_and_klass.second);
  }
}

} // namespace tl
} // namespace vk
