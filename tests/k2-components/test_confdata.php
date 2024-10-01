<?

function test_confdata_get_value(): bool {
  $existing = confdata_get_value("now");
  $missing = confdata_get_value("missing_key");
  return !is_null($existing) && is_null($missing);
}

function test_confdata_get_values_by_any_wildcard(): bool {
  $existing1 = confdata_get_values_by_any_wildcard("n");
  $existing2 = confdata_get_values_by_any_wildcard("no");
  $existing3 = confdata_get_values_by_any_wildcard("now");
  $missing = confdata_get_values_by_any_wildcard("missing_wildcard");
  return !empty($existing1) && !empty($existing2) && !empty($existing3) && empty($missing);
}

function test_confdata_get_values_by_predefined_wildcard(): bool {
  $existing1 = confdata_get_values_by_predefined_wildcard("n");
  $existing2 = confdata_get_values_by_predefined_wildcard("no");
  $existing3 = confdata_get_values_by_predefined_wildcard("now");
  $missing = confdata_get_values_by_predefined_wildcard("missing_wildcard");
  return !empty($existing1) && !empty($existing2) && !empty($existing3) && empty($missing);
}

$query = component_server_accept_query();
if (test_confdata_get_value() && test_confdata_get_values_by_any_wildcard() && test_confdata_get_values_by_predefined_wildcard()) {
  component_server_send_response($query, "OK");
} else {
  component_server_send_response($query, "FAIL");
}
