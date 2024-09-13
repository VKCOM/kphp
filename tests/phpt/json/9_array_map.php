@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
  public int $id;
}

class ArrayMap {
  public array $empty_map = [];
  /** @var int[] */
  public $int_map;
  /** @var bool[] */
  public $bool_map;
  /** @var string[] */
  public $string_map;
  /** @var float[] */
  public $float_map;
  /** @var A[] */
  public $obj_map = [];

  function init() {
    $this->int_map = ["0" => 11, "99" => 22, "88" => 33];
    $this->bool_map = ["a" => true, "b" => false];
    $this->string_map = ["2" => "foo", "c" => "bar"];
    $this->float_map = ["20" => 56.78];
    $a1 = new A;
    $a1->id = 1;
    $a2 = new A;
    $a2->id = 2;
    $this->obj_map = ["a" => $a1, "b" => $a2];
  }
}

class ArrayNullObject {
  /** @var A[] */
  public $vec = [];
}

function test_array_map_decode() {
  $json = "{\"empty_map\":[],\"int_map\":{\"0\":11,\"99\":22,\"88\":33},\"bool_map\":{\"a\":true,\"b\":false}," .
   "\"string_map\":{\"2\":\"foo\",\"c\":\"bar\"},\"float_map\":{\"20\":56.78},\"obj_map\":{\"a\":{\"id\":1},\"b\":{\"id\":2}}}";
  $obj = JsonEncoder::decode($json, "ArrayMap");
  var_dump(to_array_debug($obj));
}

function test_array_map_of_nulls_decode() {
  $json = "{\"empty_map\":[],\"int_map\":{\"0\":null,\"99\":22,\"88\":null},\"bool_map\":{\"a\":null,\"b\":null}," .
   "\"string_map\":{\"2\":null,\"c\":\"bar\"},\"float_map\":{\"20\":null},\"obj_map\":{\"a\":null,\"b\":{\"id\":null}}}";
  $obj = JsonEncoder::decode($json, "ArrayMap");
  var_dump(JsonEncoder::getLastError());
}

function test_array_map() {
  $obj = new ArrayMap;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "ArrayMap");
  var_dump(to_array_debug($restored_obj));
}

function test_array_map_null_object() {
  /** @var A[] */
  $dummy_arr = [];
  $null_a = $dummy_arr[0] ?? null;

  $a1 = new A;
  $a1->id = 77;

  $a2 = new A;
  $a2->id = 44;

  $obj = new ArrayNullObject;
  $obj->vec = ["a" => $a1, "b" => $null_a, "c" => $a2];

  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "ArrayNullObject");
  var_dump(to_array_debug($restored_obj));
}

class ArrayAsHashmap {
  /** @kphp-json array_as_hashmap */
  public array $untyped_array = [];
  /**
   * @kphp-json array_as_hashmap
   * @var int[]
   */
  public $empty_array;
  /**
   * @kphp-json array_as_hashmap
   * @var int[]
   */
  public $int_array;
  /**
   * @kphp-json array_as_hashmap
   * @var int[][]
   */
  public $int_array_2dim;
  /**
   * @var int[]
   */
  public $int_vector;
  /**
   * @kphp-json array_as_hashmap
   * @var ?int[]
   */
  public $optional_array;
  /**
   * @kphp-json array_as_hashmap
   * @var mixed[]
   */
  public $mixed_array;

  function __construct() {
    $this->empty_array = [];
    $this->int_array = [11, 22];
    $this->int_array_2dim = [[11, 22], [33]];
    $this->int_vector = [11, 22];
    $this->optional_array = [11, 22];
    $this->mixed_array = [11, 22];
  }
}

function test_array_as_hashmap() {
  $json = JsonEncoder::encode(new ArrayAsHashmap);
  var_dump($json);
  $dump = to_array_debug(JsonEncoder::decode($json, ArrayAsHashmap::class));
  var_dump($dump);
}

class Nested {
  /** @var int[] */
  public $int_array;
}

class ArrayAsHashmapNested {
  /**
   * @kphp-json array_as_hashmap
   * @var Nested[]
   */
  public $nested;

  function __construct() {
    $obj = new Nested;
    $obj->int_array = [11, 22];
    $this->nested = [$obj, $obj];
  }
}

function test_array_as_hashmap_nested() {
  $json = JsonEncoder::encode(new ArrayAsHashmapNested);
  var_dump($json);
  $dump = to_array_debug(JsonEncoder::decode($json, ArrayAsHashmapNested::class));
  var_dump($dump);
}

test_array_map_decode();
test_array_map_of_nulls_decode();
test_array_map();
test_array_map_null_object();
test_array_as_hashmap();
test_array_as_hashmap_nested();
