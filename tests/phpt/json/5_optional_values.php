@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {}

class OptionalTypes {
  public ?int $int_f;
  public ?bool $bool_f;
  public ?string $string_f;
  public ?float $float_f;
  public ?A $a_f;
  /** @var ?int[] */
  public $array_f;
}

function test_optional_decode() {
  $obj = JsonEncoder::decode("{}", "OptionalTypes");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("{\"int_f\":null,\"bool_f\":null,\"string_f\":null,\"float_f\":null,\"a_f\":null,\"array_f\":null}", "OptionalTypes");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("{\"int_f\":123,\"bool_f\":true,\"string_f\":\"foo\",\"float_f\":123.45,\"a_f\":{},\"array_f\":[33, 55]}", "OptionalTypes");
  var_dump(to_array_debug($obj));

  $obj = JsonEncoder::decode("null", "OptionalTypes");
  var_dump($obj === null);
  var_dump(JsonEncoder::getLastError());
}

function test_optional_values() {
  $obj = new OptionalTypes;
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "OptionalTypes");
  var_dump(to_array_debug($restored_obj));

  $obj = new OptionalTypes;
  $obj->int_f = null;
  $obj->bool_f = null;
  $obj->string_f = null;
  $obj->float_f = null;
  $obj->a_f = null;
  $obj->array_f = null;
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "OptionalTypes");
  var_dump(to_array_debug($restored_obj));

  $obj = new OptionalTypes;
  $obj->int_f = 123;
  $obj->bool_f = true;
  $obj->string_f = "foo";
  $obj->float_f = 123.45;
  $obj->a_f = new A;
  $obj->array_f = [33, 55];
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "OptionalTypes");
  var_dump(to_array_debug($restored_obj));
}

test_optional_decode();
test_optional_values();
