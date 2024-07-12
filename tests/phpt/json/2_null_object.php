@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class SimpleB {
  public int $b_int;
}

class SimpleA {
  public int $a_int;
  /** @var SimpleB */
  public $a_b_instance;
}

class SimpleANullable {
  public int $a_int;
  public ?SimpleB $a_b_instance;
}

function test_null_property() {
  $obj = JsonEncoder::decode("{\"b_int\":null}", "SimpleB");
  var_dump(JsonEncoder::getLastError());
}

function test_empty_object() {
  $obj = JsonEncoder::decode("{}", "SimpleB");
  var_dump(JsonEncoder::getLastError());
}

function test_null_property_2() {
  $obj = JsonEncoder::decode("{\"a_int\":99,\"a_b_instance\":{\"b_int\":null}}", "SimpleA");
  var_dump(JsonEncoder::getLastError());
}

function test_null_object() {
  $obj = JsonEncoder::decode("{\"a_int\":99,\"a_b_instance\":null}", "SimpleA");
  var_dump(JsonEncoder::getLastError());
}

function test_absent_object() {
  $obj = JsonEncoder::decode("{\"a_int\":99}", "SimpleA");
  var_dump(JsonEncoder::getLastError());
}

function test_null_object_root() {
  /** @var SimpleA[] */
  $dummy_arr = [];
  $null_a = $dummy_arr[0] ?? null;

  $json = JsonEncoder::encode($null_a);
  var_dump($json);
  $dump = JsonEncoder::decode($json, "SimpleANullable");
  if ($dump)
    throw new Exception("unexpected dump");
  var_dump(JsonEncoder::getLastError());
}

function test_null_object_inner() {
  $origin_a = new SimpleANullable;
  $origin_a->a_int = 99;

  $json = JsonEncoder::encode($origin_a);
  var_dump($json);
  $restored_obj = JsonEncoder::decode($json, "SimpleANullable");
  var_dump(to_array_debug($restored_obj));
  if (!$restored_obj || $restored_obj->a_b_instance)
    throw new Exception("unexpected restored_obj");
}


test_null_property();
test_empty_object();
test_null_property_2();
test_null_object();
test_absent_object();
test_null_object_root();
test_null_object_inner();
