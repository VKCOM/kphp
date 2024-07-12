@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class MixedValue {
  /** @var mixed */
  public $mixed_value;

  function __construct($v) { $this->mixed_value = $v; }
}

function test_mixed_decode() {
  $obj = JsonEncoder::decode("{}", "MixedValue");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("{\"mixed_value\":null}", "MixedValue");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("{\"mixed_value\":true}", "MixedValue");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("{\"mixed_value\":12345}", "MixedValue");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("{\"mixed_value\":987.65}", "MixedValue");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("{\"mixed_value\":\"foo\"}", "MixedValue");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode('{"mixed_value":[1, 2, {"k":"v","k2":22}, null]}', "MixedValue");
  var_dump(to_array_debug($obj));
}

function test_mixed_values() {
  $obj = new MixedValue(null);
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedValue");
  var_dump(to_array_debug($restored_obj));

  $obj = new MixedValue(true);
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedValue");
  var_dump(to_array_debug($restored_obj));

  $obj = new MixedValue(12345);
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedValue");
  var_dump(to_array_debug($restored_obj));

  $obj = new MixedValue(987.65);
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedValue");
  var_dump(to_array_debug($restored_obj));

  $obj = new MixedValue("foo");
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedValue");
  var_dump(to_array_debug($restored_obj));

  $obj = new MixedValue([1, 2, 'str']);
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedValue");
  var_dump(to_array_debug($restored_obj));
}

test_mixed_decode();
test_mixed_values();
