@ok
<?php
require_once 'kphp_tester_include.php';

class Foo {
  /** @kphp-json float_precision=1 */
  public float $foo1;
  /** @kphp-json float_precision=2 */
  public float $foo2;
  /** @kphp-json float_precision=3 */
  public float $foo3;
  /** @kphp-json float_precision=4 */
  public float $foo4;
  /** @kphp-json float_precision=5 */
  public float $foo5;
  /** @kphp-json float_precision=6 */
  public float $foo6;
  /** @kphp-json float_precision=7 */
  public float $foo7;
  /** @kphp-json float_precision=8 */
  public float $foo8;
  public float $foo9;

  function init() {
    $this->foo1 = 0.123456789;
    $this->foo2 = 0.123456789;
    $this->foo3 = -0.123456789;
    $this->foo4 = -0.123456789;
    $this->foo5 = 123.123456789;
    $this->foo6 = 123.123456789;
    $this->foo7 = 123.123456789;
    $this->foo8 = 123.123456789;
    $this->foo9 = 123.123456789;
  }
}

function test_float_precision() {
  $obj = new Foo;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $dump_json = to_array_debug(JsonEncoder::decode($json, Foo::class));
  var_dump($dump_json);
}

class MyJsonEncoder extends JsonEncoder {
  const float_precision = 3;
}

/** @kphp-json float_precision=2 */
class BTag {
  public float $b_foo;
  public float $b_bar;

  function init() {
    $this->b_foo = 0.123456789;
    $this->b_bar = 0.123456789;
  }
}

class ATag extends BTag {
  public float $a_foo;
  /** @kphp-json float_precision=4 */
  public float $a_bar;

  function init() {
    parent::init();
    $this->a_foo = 0.123456789;
    $this->a_bar = 0.123456789;
  }
}

function test_float_precision_with_tag() {
  $a = new ATag;
  $a->init();
  $b = new BTag;
  $b->init();
  $json_a = JsonEncoder::encode($a);
  $json_b = JsonEncoder::encode($b);
  $my_json_a = MyJsonEncoder::encode($a);
  $my_json_b = MyJsonEncoder::encode($b);

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, ATag::class));
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, BTag::class));
  $dump_my_json_a = to_array_debug(JsonEncoder::decode($my_json_a, ATag::class));
  $dump_my_json_b = to_array_debug(JsonEncoder::decode($my_json_b, BTag::class));

  var_dump($dump_json_a);
  var_dump($dump_json_b);
  var_dump($dump_my_json_a);
  var_dump($dump_my_json_b);
}

class B1 {
  public float $b_foo;
}

class A1 {
  /** @kphp-json float_precision=4 */
  public B1 $a_b;

  function init() {
    $this->a_b = new B1;
    $this->a_b->b_foo = 0.123456789;
  }
}

function test_float_precision_nested_obj() {
  $a = new A1;
  $a->init();
  $json_a = JsonEncoder::encode($a);
  $my_json_a = MyJsonEncoder::encode($a);

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A1::class));
  $dump_my_json_a = to_array_debug(JsonEncoder::decode($my_json_a, A1::class));

  var_dump($dump_json_a);
  var_dump($dump_my_json_a);
}

class A2 {
  /**
    * @kphp-json float_precision=4
    * @var B1[]
    */
  public $foo;
  /** @var float [] */
  public $bar;

  function init() {
    $b = new B1;
    $b->b_foo = 0.123456789;
    $this->foo = [$b, $b];
    $this->bar = [1.123456789, 1.123456789];
  }
}

function test_float_precision_array() {
  $a = new A2;
  $a->init();
  $json_a = JsonEncoder::encode($a);
  $my_json_a = MyJsonEncoder::encode($a);

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A2::class));
  $dump_my_json_a = to_array_debug(JsonEncoder::decode($my_json_a, A2::class));

  var_dump($dump_json_a);
  var_dump($dump_my_json_a);
}

test_float_precision();
test_float_precision_with_tag();
test_float_precision_nested_obj();
test_float_precision_array();
