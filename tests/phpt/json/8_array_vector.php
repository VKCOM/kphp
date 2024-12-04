@ok
<?php
require_once 'kphp_tester_include.php';

class A {
  public int $id;
}

class ArrayVector {
  public array $empty_vec = [];
  /** @var int[] */
  public $int_vec;
  /** @var bool[] */
  public $bool_vec;
  /** @var string[] */
  public $string_vec;
  /** @var float[] */
  public $float_vec;
  /** @var A[] */
  public $obj_vec;

  function init() {
    $this->int_vec = [11, 22, 33];
    $this->bool_vec = [true, false];
    $this->string_vec = ["foo", "bar"];
    $this->float_vec = [56.78];
    $a1 = new A;
    $a1->id = 1;
    $a2 = new A;
    $a2->id = 2;
    $this->obj_vec = [$a1, $a2];
  }
}

class ArrayNullObject {
  /** @var A[] */
  public $vec = [];
}

class ResponseInts {
    /** @var int[] */
    public $numbers;
}


function test_array_vector_decode() {
  $json = "{\"empty_vec\":[],\"int_vec\":[11,22,33],\"bool_vec\":[true,false]," .
   "\"string_vec\":[\"foo\",\"bar\"],\"float_vec\":[56.78],\"obj_vec\":[{\"id\":1},{\"id\":2}]}";
  $obj = JsonEncoder::decode($json, "ArrayVector");
  var_dump(to_array_debug($obj));
}

function test_array_vector_of_nulls_decode() {
  $json = "{\"empty_vec\":[],\"int_vec\":[null],\"bool_vec\":[null,null]," .
   "\"string_vec\":[null],\"float_vec\":[null, null],\"obj_vec\":[null, null]}";
  $obj = JsonEncoder::decode($json, "ArrayVector");
  var_dump(JsonEncoder::getLastError());
}

function test_array_vector_null_object_decode() {
  $obj = JsonEncoder::decode("{\"vec\":[{\"id\":1},null]}", "ArrayNullObject");
  var_dump(to_array_debug($obj));
}

function test_response_ints_decode() {
  $obj = JsonEncoder::decode('{"numbers":[1,2,null,4]}', ResponseInts::class);
  var_dump(JsonEncoder::getLastError());
}


function test_array_vector() {
  $obj = new ArrayVector;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "ArrayVector");
  var_dump(to_array_debug($restored_obj));
}

function test_array_vector_null_object() {
  /** @var A[] */
  $dummy_arr = [];
  $null_a = $dummy_arr[0] ?? null;

  $a1 = new A;
  $a1->id = 77;

  $a2 = new A;
  $a2->id = 44;

  $obj = new ArrayNullObject;
  $obj->vec = [$a1, $null_a, $a2];

  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "ArrayNullObject");
  var_dump(to_array_debug($restored_obj));
}

class Foo {
  /** @var int[] */
  public $foo;

  function init() {
    $this->foo = [0 => 11, "1" => 22, 2 => 33];
  }
}

function test_array_pseudo_vector() {
  $obj = new Foo;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, Foo::class);
  var_dump(to_array_debug($restored_obj));
}

class Response {
    public array $ids = [1];
    public array $numValues = [1,2,3];
}

function test_array_non_empty_defaults() {
    $r = JsonEncoder::decode('{"ids":[]}', Response::class);
    var_dump(to_array_debug($r));

    $r = new Response;
    $r->numValues = [100];
    $json = JsonEncoder::encode($r);
    echo $json, "\n";
    $restored_obj = JsonEncoder::decode($json, Response::class);
    var_dump(to_array_debug($restored_obj));
}


test_array_vector_decode();
test_array_vector_of_nulls_decode();
test_array_vector_null_object_decode();
test_response_ints_decode();
test_array_vector();
test_array_vector_null_object();
test_array_pseudo_vector();
test_array_non_empty_defaults();
