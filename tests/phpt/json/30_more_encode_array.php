@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class B {
  public string $b_foo = "b_foo";
  /** @var int[] */
  public $int_arr = [0];
}

class A extends B {
  public string $a_foo = "a_ff";
}


function test_more() {
  $json = JsonEncoder::encode(new A, 0, ["key1" => "value1", "key2" => 77]);
  $restored_obj = JsonEncoder::decode($json, A::class);
  var_dump($json);
  var_dump(to_array_debug($restored_obj));
}


test_more();
