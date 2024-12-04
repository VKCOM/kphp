@ok
<?php
require_once 'kphp_tester_include.php';

class EmptyClass {}

class C {
  public string $c_foo = "c_foo";
  /** @var int[] */
  public $c_arr = [11, 22];
  public EmptyClass $c_empty_obj;

  function __construct() {
    $this->c_empty_obj = new EmptyClass;
  }
}

class B {
  public string $b_foo = "b_foo";
  /** @var C[] */
  public $b_arr = [];
  /** @var C[] */
  public $b_empty_arr = [];

  function __construct() {
    $this->b_arr = [new C, new C];
  }
}

class D {
  public string $d_foo = "d_foo";
  /** @var C[] */
  public $d_arr;
  /** @var C[] */
  public $d_empty_arr = [];

  function __construct() {
    $this->d_arr = [new C];
  }
}

class A extends B {
  public string $a_foo = "a_ff";
  /** @var ?C */
  public $a_bar_uninited;
  public D $d_bar;
  public EmptyClass $a_empty_obj;

  function __construct() {
    $this->d_bar = new D;
    $this->a_empty_obj = new EmptyClass;
  }
}

class Input {
  public string $a;
  public float $b;
  public bool $c;
}


function test_input_pretty_json_format_ok() {
  $json = "{\n\t\"a\": \"foo\",\n\t\"b\": 12.34,\n\t\"c\": true\n}";
  $obj = JsonEncoder::decode($json, Input::class);
  if (!$obj) throw new Exception("unexpected null");
  var_dump(to_array_debug($obj));
}

function test_pretty_print() {
  $json_a = JsonEncoder::encode(new A, JSON_PRETTY_PRINT);
  $obj_a = to_array_debug(JsonEncoder::decode($json_a, A::class));
  var_dump($json_a);
  var_dump($obj_a);

  $json_b = JsonEncoder::encode(new B, JSON_PRETTY_PRINT);
  $obj_b = to_array_debug(JsonEncoder::decode($json_b, B::class));
  var_dump($json_b);
  var_dump($obj_b);

  $json_c = JsonEncoder::encode(new C, JSON_PRETTY_PRINT);
  $obj_c = to_array_debug(JsonEncoder::decode($json_c, C::class));
  var_dump($json_c);
  var_dump($obj_c);

  $json_e = JsonEncoder::encode(new EmptyClass, JSON_PRETTY_PRINT);
  $obj_e = to_array_debug(JsonEncoder::decode($json_e, EmptyClass::class));
  var_dump($json_e);
  var_dump($obj_e);
}

test_input_pretty_json_format_ok();
test_pretty_print();
