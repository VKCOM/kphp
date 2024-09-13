@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
  /** @var string[] */
  public $map;
  function init() {
    $this->map = [77 => "foo", 88 => "bar", "99" => "baz", "key" => "value"];
  }
}


function test_array_string_int_key_conversion_decode() {
  $json = "{\"map\":{\"77\":\"foo\",\"88\":\"bar\",\"99\":\"baz\",\"key\":\"value\"}}";
  $obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($obj));
}

function test_array_string_int_key_conversion() {
  $obj = new A;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($restored_obj));
}

test_array_string_int_key_conversion_decode();
test_array_string_int_key_conversion();
