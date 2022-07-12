@ok
<?php
require_once 'kphp_tester_include.php';

class AInt {
  public int $int_f;
}

function test_conversion_to_int_errors() {
  var_dump(JsonEncoder::decode('{"int_f":12.34}', AInt::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"int_f":true}', AInt::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"int_f":"foo"}', AInt::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"int_f":{}}', AInt::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"int_f":[]}', AInt::class) === null);
  var_dump(JsonEncoder::getLastError());
}

/** @kphp-json flatten */
class MyInt {
  public int $field;
}

class BInt {
  public MyInt $b;
}

function test_flatten_class_int_errors() {
  var_dump(JsonEncoder::decode('{"b":true}', BInt::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('true', MyInt::class) === null);
  var_dump(JsonEncoder::getLastError());
}

test_conversion_to_int_errors();
test_flatten_class_int_errors();


class AFloat {
  public float $float_f;
}

function test_conversion_to_float_errors() {
  var_dump(JsonEncoder::decode('{"float_f":12}', AFloat::class) !== null);
  if(JsonEncoder::getLastError() !== '') throw new Exception("not empty int->float");

  var_dump(JsonEncoder::decode('{"float_f":true}', AFloat::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"float_f":"fo\"o"}', AFloat::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"float_f":{}}', AFloat::class) === null);
  var_dump(JsonEncoder::getLastError());
}

test_conversion_to_float_errors();


class ABool {
  public bool $bool_f;
}

function test_conversion_to_bool_errors() {
  var_dump(JsonEncoder::decode('{"bool_f":12}', ABool::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"bool_f":12.34}', ABool::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"bool_f":null}', ABool::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"bool_f":[]}', ABool::class) === null);
  var_dump(JsonEncoder::getLastError());
}

test_conversion_to_bool_errors();


class AString {
  public string $string_f;
}

function test_conversion_to_string_errors() {
  var_dump(JsonEncoder::decode('{"string_f":12}', AString::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"string_f":true}', AString::class) === null);
  var_dump(JsonEncoder::getLastError());
}

test_conversion_to_string_errors();


class B1 {}

class AObject {
  public B1 $object_f;
}

function test_conversion_to_object_errors() {
  var_dump(JsonEncoder::decode('{"object_f":12}', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"object_f":12.34}', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"object_f":false}', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());
}

function test_root_object_errors() {
  var_dump(JsonEncoder::decode('null', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('false', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('12', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('12.34', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('"foo"', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('[]', AObject::class) === null);
  var_dump(JsonEncoder::getLastError());
}

class C {
  public string $foo = 'foo';
}

class DObject {
  public C $bar;
}

function test_nested_object_errors() {
  var_dump(JsonEncoder::decode('{"bar":{"foo":7}}', DObject::class) === null);
  var_dump(JsonEncoder::getLastError());
}

test_conversion_to_object_errors();
test_root_object_errors();
test_nested_object_errors();


class AArray {
  /** @var int[] */
  public $array_f;
}

function test_conversion_to_array_errors() {
  var_dump(JsonEncoder::decode('{"array_f":12}', AArray::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"array_f":12.34}', AArray::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"array_f":true}', AArray::class) === null);
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::decode('{"array_f":[[1]]}', AArray::class) === null);
  var_dump(JsonEncoder::getLastError());
}

test_conversion_to_array_errors();
