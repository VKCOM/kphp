@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/** @kphp-json flatten */
class MyValue {
  public int $value;
}

class A {
  public MyValue $single_value;
  /** @var MyValue[] */
  public $map;
  /** @var MyValue[] */
  public $vector;

  function init() {
    $this->single_value = new MyValue;
    $this->single_value->value = 11;

    $a = new MyValue;
    $a->value = 33;
    $b = new MyValue;
    $b->value = 55;
    $this->map = ["a" => $a, "b" => $b];

    $one = new MyValue;
    $one->value = 44;
    $two = new MyValue;
    $two->value = 99;
    $this->vector = [$one, $two];
  }
}

function test_flatten_classes() {
  $a = new A;
  $a->init();
  $v = new MyValue;
  $v->value = 42;
  $json_a = JsonEncoder::encode($a);
  echo $json_a, "\n";
  $json_v = JsonEncoder::encode($v);
  echo $json_v, "\n";
  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A::class));
  $dump_json_v = to_array_debug(JsonEncoder::decode($json_v, MyValue::class));
  var_dump($dump_json_a);
  var_dump($dump_json_v);
}

/** @kphp-json flatten */
class FlatMixed {
  /** @var mixed */
  public $value;
}
/** @kphp-json flatten */
class FlatOptional {
  public ?int $value;
}
/** @kphp-json flatten */
class FlatOptionalMixed {
  /** @var ?mixed */
  public $value;
}

class B {
  public FlatMixed $a;
  public FlatOptional $b;
  public FlatOptionalMixed $c;

  function init() {
    $this->a = new FlatMixed;
    $this->a->value = "foo";
    $this->b = new FlatOptional;
    $this->b->value = 42;
    $this->c = new FlatOptionalMixed;
    $this->c->value = 23;
  }
}

function test_flatten_classes_mixed_optional_types() {
  $b = new B;
  $b->init();
  $json_b = JsonEncoder::encode($b);
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, B::class));
  var_dump($json_b);
  var_dump($dump_json_b);
}

/** @kphp-json flatten */
class MyVector {
  /** @var int[] */
  public $v;
  function __construct($v) {
    $this->v = $v;
  }
}

/** @kphp-json flatten */
class MyHashmap {
  /**
    * @var int[]
    * @kphp-json array_as_hashmap
    */
  public $h;
  function __construct($h) {
    $this->h = $h;
  }
}

class C {
  public MyVector $v;
  public MyHashmap $h;

  function __construct() {
    $this->v = new MyVector([11, 22]);
    $this->h = new MyHashmap([33, 44]);
  }
}

function test_flatten_array_as_hashmap() {
  $json_c = JsonEncoder::encode(new C);
  $json_v = JsonEncoder::encode(new MyVector([55, 77]));
  $json_h = JsonEncoder::encode(new MyHashmap([88, 99]));
  var_dump($json_c);
  var_dump($json_v);
  var_dump($json_h);
  $dump_json_c = to_array_debug(JsonEncoder::decode($json_c, C::class));
  $dump_json_v = to_array_debug(JsonEncoder::decode($json_v, MyVector::class));
  $dump_json_h = to_array_debug(JsonEncoder::decode($json_h, MyHashmap::class));
  var_dump($dump_json_c);
  var_dump($dump_json_v);
  var_dump($dump_json_h);
}

/**
 * @kphp-json rename_policy = none
 * @kphp-json flatten = false
 */
class Dummy {
  public string $foo = 'bar';
}

/** @kphp-json flatten */
class MyObject {
  public Dummy $o;

  function __construct($o) {
    $this->o = $o;
  }
}

class D {
  public MyObject $o;
  public MyVector $empty_v;
  public MyHashmap $empty_h;

  function __construct() {
    $this->o = new MyObject(new Dummy);
    $this->empty_v = new MyVector([]);
    $this->empty_h = new MyHashmap([]);
  }
}

function test_flatten_empty_arr_obj() {
  $json_d = JsonEncoder::encode(new D);
  $json_o = JsonEncoder::encode(new MyObject(new Dummy));
  $json_v = JsonEncoder::encode(new MyVector([]));
  $json_h = JsonEncoder::encode(new MyHashmap([]));
  var_dump($json_d);
  var_dump($json_o);
  var_dump($json_v);
  var_dump($json_h);
  $dump_json_d = to_array_debug(JsonEncoder::decode($json_d, D::class));
  $dump_json_o = to_array_debug(JsonEncoder::decode($json_o, MyObject::class));
  $dump_json_v = to_array_debug(JsonEncoder::decode($json_v, MyVector::class));
  $dump_json_h = to_array_debug(JsonEncoder::decode($json_h, MyHashmap::class));
  var_dump($dump_json_d);
  var_dump($dump_json_o);
  var_dump($dump_json_v);
  var_dump($dump_json_h);
}

/** @kphp-json flatten */
class MyBool {
  public bool $field;

  function __construct($field) {
    $this->field = $field;
  }
}

/** @kphp-json flatten */
class MyFloat {
  /** @kphp-json float_precision = 2 */
  public float $field;

  function __construct($field) {
    $this->field = $field;
  }
}

/** @kphp-json flatten = true */
class MyString {
  public string $field;

  function __construct($field) {
    $this->field = $field;
  }
}


/** @kphp-json flatten */
class MyOptionalString {
  public ?string $field;

  function __construct($field) {
    $this->field = $field;
  }
}

function test_root_flatten_class() {
  $json_b = JsonEncoder::encode(new MyBool(false));
  $json_f = JsonEncoder::encode(new MyFloat(1.1234));
  $json_s = JsonEncoder::encode(new MyString("abc"));
  $json_os1 = JsonEncoder::encode(new MyOptionalString(null));
  $json_os2 = JsonEncoder::encode(new MyOptionalString("foo"));
  var_dump($json_b);
  var_dump($json_f);
  var_dump($json_s);
  var_dump($json_os1);
  var_dump($json_os2);
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, MyBool::class));
  $dump_json_f = to_array_debug(JsonEncoder::decode($json_f, MyFloat::class));
  $dump_json_s = to_array_debug(JsonEncoder::decode($json_s, MyString::class));
  $dump_json_os1 = to_array_debug(JsonEncoder::decode($json_os1, MyOptionalString::class));
  $dump_json_os2 = to_array_debug(JsonEncoder::decode($json_os2, MyOptionalString::class));
  var_dump($dump_json_b);
  var_dump($dump_json_f);
  var_dump($dump_json_s);
  var_dump($dump_json_os1);
  var_dump($dump_json_os2);
}

/** @kphp-json flatten */
class MyFlatOuter {
  public int $field;

  function __construct($field) {
    $this->field = $field;
  }
}

/** @kphp-json flatten */
class MyFlatInner {
  public MyFlatOuter $outer;

  function __construct(int $field) {
    $this->outer = new MyFlatOuter($field);
  }
}

class E {
  public MyFlatInner $inner;

  function __construct(int $field) {
    $this->inner = new MyFlatInner($field);
  }
}

function test_nested_flatten_classes() {
  $json = JsonEncoder::encode(new E(42));
  var_dump($json);
  $dump = to_array_debug(JsonEncoder::decode($json, E::class));
  var_dump($dump);
  var_dump(JsonEncoder::getLastError());

  $json = JsonEncoder::encode(new MyFlatInner(23));
  var_dump($json);
  $dump = to_array_debug(JsonEncoder::decode($json, MyFlatInner::class));
  var_dump($dump);
  var_dump(JsonEncoder::getLastError());
}

class MyJsonEncoder extends JsonEncoder {
  const skip_if_default = true;
  const visibility_policy = 'public';
}

/** @kphp-json flatten */
class MyFlatten {
  private int $value = 42;
}

class F {
  public MyFlatten $v;

  function __construct() {
    $this->v = new MyFlatten;
  }
}

function test_ignore_encoder_constants_for_flatten_class() {
  $json_m = MyJsonEncoder::encode(new MyFlatten);
  $json_f = MyJsonEncoder::encode(new F);
  var_dump($json_m);
  var_dump($json_f);
  $dump_m = to_array_debug(MyJsonEncoder::decode($json_m, MyFlatten::class));
  $dump_f = to_array_debug(MyJsonEncoder::decode($json_f, F::class));
  var_dump($dump_m);
  var_dump($dump_f);
  var_dump(JsonEncoder::getLastError());
}

/** @kphp-json flatten */
class ResponseMaybeNull {
    /** @var ?int[] */
    public $numbers = [];
}

function test_root_null() {
    $resp = JsonEncoder::decode('null', ResponseMaybeNull::class);
    var_dump(to_array_debug($resp));
}


/** @kphp-json flatten */
class IntWrapper {
    public int $value;

    function __construct(int $v) { $this->value = $v; }
}

class ResponseIntWrappers {
    /** @var IntWrapper[] */
    public $numbers = [];
}

function test_invalid_instead_of_int_in_flatten() {
    $json = '{"numbers":[1,2,"str"]}';
    $restored_obj = JsonEncoder::decode($json, ResponseIntWrappers::class);
    var_dump(JsonEncoder::getLastError());

    $json = '{"numbers":[1,2,null]}';
    $restored_obj = JsonEncoder::decode($json, ResponseIntWrappers::class);
    var_dump(JsonEncoder::getLastError());
}


/** @kphp-json flatten */
class OptIntWrapper {
    public ?int $value;

    function __construct(?int $v) { $this->value = $v; }
}

class ResponseOptIntWrappers {
    /** @var OptIntWrapper[] */
    public $numbers = [];
}

function test_null_as_opt_in_flatten() {
    $json = '{"numbers":[1,2,null]}';
    $restored_obj = JsonEncoder::decode($json, ResponseOptIntWrappers::class);
    var_dump(to_array_debug($restored_obj));
}

/** @kphp-json flatten */
class MyFlat {
  /** @kphp-json raw_string */
  public string $anything;

  public function __construct(string $anything) {
    $this->anything = $anything;
  }
}

class ContainerOfMyFlats {
  public MyFlat $flat;
  /** @var MyFlat[] */
  public array $arr_flat;
}

function test_flatten_and_raw_string() {
  $json = JsonEncoder::encode(new MyFlat('[1]'));
  echo $json, "\n";
  $restored_obj = JsonEncoder::decode($json, MyFlat::class);
  var_dump(to_array_debug($restored_obj));

  $c = new ContainerOfMyFlats();
  $c->flat = new MyFlat('"str"');
  $c->arr_flat[] = new MyFlat('null');
  $c->arr_flat[] = new MyFlat('{"hello":[],"world":{}}');
  $json = JsonEncoder::encode($c);
  echo $json, "\n";
  $back = JsonEncoder::decode($json, ContainerOfMyFlats::class);
  var_dump(to_array_debug($back));
}


test_flatten_classes();
test_flatten_classes_mixed_optional_types();
test_flatten_array_as_hashmap();
test_flatten_empty_arr_obj();
test_root_flatten_class();
test_nested_flatten_classes();
test_ignore_encoder_constants_for_flatten_class();
test_root_null();
test_invalid_instead_of_int_in_flatten();
test_null_as_opt_in_flatten();
test_flatten_and_raw_string();
