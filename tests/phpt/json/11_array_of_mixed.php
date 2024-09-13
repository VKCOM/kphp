@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class MixedArrays {
  /** @var mixed */
  public $empty_vec;
  /** @var mixed */
  public $int_vec;
  /** @var mixed */
  public $bool_vec;
  /** @var mixed */
  public $string_vec;
  /** @var mixed */
  public $float_vec;

  function init() {
   $this->empty_vec = [];
   $this->int_vec = ["a" => 11, "b" => 22, "c" => 33];
   $this->bool_vec = [true, false];
   $this->string_vec = ["a" => "foo", "b" => "bar"];
   $this->float_vec = [56.78];
  }
}

class MixedArrayOfArrays {
  /** @var mixed */
  public $arr;

  function init() {
    $this->arr[] = [11, 22];
    $this->arr[] = [33, 44];
  }
}

function test_array_of_null_mixed_decode() {
  $json = "{}";
  $obj = JsonEncoder::decode($json, "MixedArrays");
  var_dump(to_array_debug($obj));

  $json = "{\"empty_vec\":null,\"int_vec\":null,\"bool_vec\":null," .
    "\"string_vec\":null,\"float_vec\":null}";
  $obj = JsonEncoder::decode($json, "MixedArrays");
  var_dump(to_array_debug($obj));
}

function test_array_of_mixed_decode() {
  $json = "{\"empty_vec\":[],\"int_vec\":{\"a\":11, \"b\":22, \"c\":33},\"bool_vec\":[true,false]," .
    "\"string_vec\":{\"a\":\"foo\", \"b\":\"bar\"},\"float_vec\":[56.78]}";
  $obj = JsonEncoder::decode($json, "MixedArrays");
  var_dump(to_array_debug($obj));
}

function test_array_of_arrays_mixed_decode() {
  $json = "{\"arr\":[[11,22],[33,44]]}";
  $obj = JsonEncoder::decode($json, "MixedArrayOfArrays");
  var_dump(to_array_debug($obj));
}

function test_array_of_null_mixed() {
  $obj = new MixedArrays;
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedArrays");
  var_dump(to_array_debug($restored_obj));

  $obj = new MixedArrays;
  $obj->empty_vec = null;
  $obj->int_vec = null;
  $obj->bool_vec = null;
  $obj->string_vec = null;
  $obj->float_vec = null;
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedArrays");
  var_dump(to_array_debug($restored_obj));
}

function test_array_of_mixed() {
  $obj = new MixedArrays;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedArrays");
  var_dump(to_array_debug($restored_obj));
}

function test_array_of_arrays_mixed() {
  $obj = new MixedArrayOfArrays;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "MixedArrayOfArrays");
  var_dump(to_array_debug($restored_obj));
}

test_array_of_null_mixed_decode();
test_array_of_mixed_decode();
test_array_of_arrays_mixed_decode();
test_array_of_null_mixed();
test_array_of_mixed();
test_array_of_arrays_mixed();
