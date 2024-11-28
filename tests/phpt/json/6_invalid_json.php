@ok
<?php
require_once 'kphp_tester_include.php';

class A {}

function clear_error() {
  JsonEncoder::decode('{}', A::class);
  var_dump(JsonEncoder::getLastError() == '');
}

function test_no_errors() {
  var_dump(JsonEncoder::getLastError() == '');
  var_dump(JsonEncoder::decode('{}', A::class) !== null);
  var_dump(JsonEncoder::getLastError() == '');
  var_dump(JsonEncoder::encode(new A) == '{}');
  var_dump(JsonEncoder::getLastError() == '');
}

function test_invalid_json() {
  var_dump(JsonEncoder::decode('', A::class) === null);
  var_dump(JsonEncoder::getLastError());
  clear_error();

  var_dump(JsonEncoder::decode('{"where_my_value":}', A::class) === null);
  var_dump(JsonEncoder::getLastError() !== '');
  clear_error();

  var_dump(JsonEncoder::decode('{"unclosed_object":"value"', A::class) === null);
  var_dump(JsonEncoder::getLastError() !== '');
  clear_error();
}

function test_root_not_object() {
  var_dump(JsonEncoder::decode('"foo"', A::class) === null);
  var_dump(JsonEncoder::getLastError());
  clear_error();

  var_dump(JsonEncoder::decode('[]', A::class) === null);
  var_dump(JsonEncoder::getLastError() != '');
  clear_error();
}

function test_next_successful_decode_clear_error() {
  var_dump(JsonEncoder::decode('malformed json string', A::class) === null);
  var_dump(JsonEncoder::getLastError() != '');
  var_dump(JsonEncoder::decode('{}', A::class) !== null);
  var_dump(JsonEncoder::getLastError() == '');
}

function test_next_successful_encode_clear_error() {
  var_dump(JsonEncoder::decode('malformed json string', A::class) === null);
  var_dump(JsonEncoder::getLastError() != '');
  var_dump(JsonEncoder::encode(new A) == '{}');
  var_dump(JsonEncoder::getLastError() == '');
}

class MyJsonEncoder extends JsonEncoder {}

function test_all_encoders_share_same_error() {
  var_dump(JsonEncoder::decode('malformed json string', A::class) === null);
  var_dump(JsonEncoder::getLastError() === MyJsonEncoder::getLastError());

  clear_error();
  var_dump(JsonEncoder::getLastError() == '' && MyJsonEncoder::getLastError() == '');

  var_dump(MyJsonEncoder::decode('malformed json string', A::class) === null);
  var_dump(JsonEncoder::getLastError() === MyJsonEncoder::getLastError());
  clear_error();
}

test_no_errors();
test_invalid_json();
test_root_not_object();
test_next_successful_decode_clear_error();
test_next_successful_encode_clear_error();
test_all_encoders_share_same_error();
