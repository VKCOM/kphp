@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class C {
  public int $idx = 0;
  function init($i) {
    $this->idx = $i;
  }
}

class B {
  /** @var C[] */
  public $arr_of_c;
  function init($i) {
    $c1 = new C;
    $c1->init($i);
    $c2 = new C;
    $c2->init($i * 10);
    $this->arr_of_c = [$c1, $c2];
  }
}

class A {
  /** @var B[] */
  public $arr_of_b;
  function init() {
    $b1 = new B;
    $b1->init(1);
    $b2 = new B;
    $b2->init(2);
    $this->arr_of_b = ["a" => $b1, "b" => $b2];
  }
}

class D {
  /** @var C[][] */
  public $arr;
  function init() {

    $c1 = new C;
    $c1->init(1);

    $c2 = new C;
    $c2->init(2);

    $c3 = new C;
    $c3->init(3);

    $c4 = new C;
    $c4->init(4);

    $this->arr[] = [$c1, $c2];
    $this->arr[] = [$c3, $c4];
  }
}


function test_array_nested_decode() {
  $json = "{\"arr_of_b\":{\"a\":{\"arr_of_c\":[{\"idx\":11},{\"idx\":22}]},\"b\":{\"arr_of_c\":[{\"idx\":33},{\"idx\":44}]}}}";
  $obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($obj));
}

function test_2_dimensional_array_decode() {
  $json = "{\"arr\":[[{\"idx\":1},{\"idx\":2}],[{\"idx\":3},{\"idx\":4}]]}";
  $obj = JsonEncoder::decode($json, "D");
  var_dump(to_array_debug($obj));
}

function test_array_nested() {
  $obj = new A;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($restored_obj));
}

function test_2_dimensional_array() {
  $obj = new D;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "D");
  var_dump(to_array_debug($restored_obj));
}

test_array_nested_decode();
test_2_dimensional_array_decode();
test_array_nested();
test_2_dimensional_array();
