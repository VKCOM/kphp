@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
  /** @kphp-json raw_string */
    public string $str;
    function __construct($s) {
      $this->str = $s;
    }
}

function test_raw_string_root_simple_types() {
  $json = JsonEncoder::encode(new A("true"));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);

  $json = JsonEncoder::encode(new A("42"));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);

  $json = JsonEncoder::encode(new A("1.23"));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);

  $json = JsonEncoder::encode(new A("{}"));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);

  $json = JsonEncoder::encode(new A("[]"));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);
}

function test_raw_string_root_array() {
  $json = JsonEncoder::encode(new A('[null,17,true,{},[]]'));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);
}

function test_raw_string_root_object() {
  $json = JsonEncoder::encode(new A('{"a":null,"b":17,"c":{},"d":[]}'));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);
}

function test_raw_string_object() {
  $json = JsonEncoder::encode(new A('{"a":{"b":18,"c":[11,22]}}'));
  $dump = to_array_debug(JsonEncoder::decode($json, A::class));
  var_dump($json);
  var_dump($dump);
}

class B {
  public string $foo;
  public int $bar;
  function __construct($f, $b) {
    $this->foo = $f;
    $this->bar = $b;
  }
}

function test_raw_string_regress() {
  $json_b = JsonEncoder::encode(new B("foo_value", 42));
  $json_a = JsonEncoder::encode(new A($json_b));
  $restored_obj_a = JsonEncoder::decode($json_a, A::class);
  $restored_obj_b = JsonEncoder::decode($restored_obj_a->str, B::class);
  $dump_a = to_array_debug($restored_obj_a);
  $dump_b = to_array_debug($restored_obj_b);
  var_dump($json_a);
  var_dump($dump_a);
}

test_raw_string_root_simple_types();
test_raw_string_root_array();
test_raw_string_root_object();
test_raw_string_object();
test_raw_string_regress();
