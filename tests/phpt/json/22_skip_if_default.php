@ok
<?php
require_once 'kphp_tester_include.php';

class Qux {}

/** @kphp-json skip_if_default */
class BasicTypes {
  public int $int_f = 123;
  /** @kphp-json skip_if_default = false */
  public int $int_unskipped_1 = 1;
  /** @kphp-json skip = false */
  public int $int_unskipped_2 = 2;
  public int $int_uninited_f;
  public float $float_f = 123.45;
  public float $float_uninited_f;
  public bool $bool_f = true;
  public bool $bool_uninited_f;
  public string $string_f = "foo";
  public string $string_uninited_f;
  /** @var int[] */
  public $arr_f = [1, 2];
  /** @var int[] */
  public $arr_uninited_f;
  /** @var Qux */
  public $obj_f;

  function change_values() {
    $this->int_f = 234;
    $this->int_uninited_f = 345;
    $this->float_f = 12.34;
    $this->float_uninited_f = 23.45;
    $this->bool_f = false;
    $this->bool_uninited_f = true;
    $this->string_f = "bar";
    $this->string_uninited_f = "baz";
    $this->arr_f = [3, 4];
    $this->arr_uninited_f = [5, 6];
    $this->obj_f = new Qux;
  }
}

function test_skip_if_default() {
  $obj = new BasicTypes;
  $json = JsonEncoder::encode($obj);
  $obj->change_values();
  $json2 = JsonEncoder::encode($obj);

  $dump_json = to_array_debug(JsonEncoder::decode($json, BasicTypes::class));
  $dump_json2 = to_array_debug(JsonEncoder::decode($json2, BasicTypes::class));

  var_dump($dump_json);
  var_dump($dump_json2);
}

class MyJsonEncoder extends JsonEncoder {
  const skip_if_default = true;
}

/** @kphp-json skip_if_default */
class BTag {
  public string $b_foo = "b_foo";
  public int $b_bar = 23;
}

class ATag extends BTag {
  public string $a_foo = "a_foo";
  /** @kphp-json skip_if_default */
  public int $a_bar = 77;
}

function test_skip_if_default_with_tag() {
  $a = new ATag;
  $b = new BTag;
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

function test_skip_if_default_changed_values_with_tag() {
  $a = new ATag;
  $a->b_bar = 42;
  $b = new BTag;
  $b->b_bar = 42;
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

class Foo {
  /**
    * @kphp-json rename=new_bar
    * @kphp-json skip_if_default
    */
  public string $bar = "bar";
}

function test_both_skip_and_rename() {
  $json = JsonEncoder::encode(new Foo);
  $dump = to_array_debug(JsonEncoder::decode($json, Foo::class));
  var_dump($json);
  var_dump($dump);
}

function test_both_skip_and_rename_change_value() {
  $foo = new Foo;
  $foo->bar = "baz";
  $json = JsonEncoder::encode($foo);
  $dump = to_array_debug(JsonEncoder::decode($json, Foo::class));
  var_dump($json);
  var_dump($dump);
}

test_skip_if_default();
test_skip_if_default_with_tag();
test_skip_if_default_changed_values_with_tag();
test_both_skip_and_rename();
test_both_skip_and_rename_change_value();
