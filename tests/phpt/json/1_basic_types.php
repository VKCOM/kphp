@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class EmptyClass {}

class BasicTypes {
  public int $int_positive_f;
  public int $int_negative_f;
  public bool $bool_true_f;
  public bool $bool_false_f;
  public string $string_f;
  public float $float_positive_f;
  public float $float_negative_f;

  function init() {
    $this->int_positive_f = 123;
    $this->int_negative_f = -42;
    $this->bool_true_f = true;
    $this->bool_false_f = false;
    $this->string_f = "foo";
    $this->float_positive_f = 123.45;
    $this->float_negative_f = -98.76;
  }

  /** @param BasicTypes $other */
  public function is_equal($other) {
    var_dump($other->int_positive_f);
    var_dump($other->int_negative_f);
    var_dump($other->bool_true_f);
    var_dump($other->bool_false_f);
    var_dump($other->string_f);
    var_dump($other->float_positive_f);
    var_dump($other->float_negative_f);

    return $this->int_positive_f === $other->int_positive_f
        && $this->int_negative_f === $other->int_negative_f
        && $this->bool_true_f === $other->bool_true_f
        && $this->bool_false_f === $other->bool_false_f
        && $this->string_f === $other->string_f
        && $this->float_positive_f == $other->float_positive_f
        && $this->float_negative_f == $other->float_negative_f;
  }
}

function test_empty_class() {
  $json = JsonEncoder::encode(new EmptyClass);
  if ($json !== '{}')
    throw new Exception("unexpected !=");
  $restored_obj = JsonEncoder::decode($json, "EmptyClass");
  if ($restored_obj === null)
    throw new Exception("unexpected null");
  var_dump(to_array_debug($restored_obj));
}

function test_basic_types() {
  $obj = new BasicTypes;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "BasicTypes");
  if(!$obj->is_equal($restored_obj))
    throw new Exception("unexpected !equal");
}

function test_decode_empty() {
  $obj = JsonEncoder::decode("{}", "EmptyClass");
  if (JsonEncoder::getLastError())
    throw new Exception("unexpected getLastError");
}

function test_decode_full() {
  $json = "{\"int_positive_f\":123,\"int_negative_f\":-42,\"bool_true_f\":true,\"bool_false_f\":false," .
            "\"string_f\":\"foo\",\"float_positive_f\":123.45,\"float_negative_f\":-98.76}";
  $obj = JsonEncoder::decode($json, "BasicTypes");
  var_dump(to_array_debug($obj));
  if ($obj->int_negative_f !== -42 || $obj->string_f !== 'foo')
    throw new Exception("unexpected !=");
}

function test_decode_null() {
  $json = "{\"int_positive_f\":null,\"int_negative_f\":null,\"bool_true_f\":null,\"bool_false_f\":null," .
            "\"string_f\":null,\"float_positive_f\":null,\"float_negative_f\":null}";
  $obj = JsonEncoder::decode($json, "BasicTypes");
  var_dump(JsonEncoder::getLastError());
  if (!JsonEncoder::getLastError())
    throw new Exception("unexpected not getLastError");
}

test_empty_class();
test_basic_types();
test_decode_empty();
test_decode_full();
test_decode_null();
