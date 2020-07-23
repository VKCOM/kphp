<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--confdata-blacklist" => "^blacklisted\.\w+$",
    "--confdata-predefined-wildcard" => [
      "a",
      "abc",
      "prefix_1",
      "prefix_a",
      "prefix_abc",
      "prefix.xyz",
      "prefix.",
    ]
  ];
}

/**
 * @kphp-infer
 * @return mixed
 */
function handle() {
  switch($_SERVER["PHP_SELF"]) {
    case "/get_value":
      return confdata_get_value($_SERVER["QUERY_STRING"]);
    case "/get_values_by_any_wildcard":
      return confdata_get_values_by_any_wildcard($_SERVER["QUERY_STRING"]);
    case "/get_values_by_predefined_wildcard":
      return confdata_get_values_by_predefined_wildcard($_SERVER["QUERY_STRING"]);
    default:
      return "unkown";
  }
}

echo json_encode(["result" => handle()]);
