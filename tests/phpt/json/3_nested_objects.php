@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
  public int $a_int;
}

class Nested {
  public int $nested_int;
}

class B {
  public int $b_int;
  public Nested $b_nested;
}

class C extends A {
  public int $c_int;
}

class NestedInParent {
  public int $nested_int = 0;
}

class AParent {
    public int $id = 1;
    public NestedInParent $n;

    function __construct() { $this->n = new NestedInParent; }
}

class BChild extends AParent {
    public int $b = 2;
}

function test_decode_nested_types() {
  $obj = JsonEncoder::decode("{\"b_int\":99,\"b_nested\":{\"nested_int\":88}}", "B");
  var_dump(to_array_debug($obj));
}

function test_decode_inheritance() {
  $obj = JsonEncoder::decode("{\"c_int\":77,\"a_int\":44}", "C");
  var_dump(to_array_debug($obj));
}

function test_nested_types() {
  $obj = new B;
  $obj->b_int = 99;
  $obj->b_nested = new Nested;
  $obj->b_nested->nested_int = 88;

  $json = JsonEncoder::encode($obj);
  echo $json, "\n";
  $restored_obj = JsonEncoder::decode($json, "B");
  var_dump(to_array_debug($restored_obj));
  if (to_array_debug($obj) !== to_array_debug($restored_obj))
    throw new Exception("unexpected !=");

  $json = JsonEncoder::encode(new BChild);
  echo $json, "\n";
  $restored_obj = JsonEncoder::decode($json, "BChild");
  var_dump(to_array_debug($restored_obj));
  if (to_array_debug(new BChild) !== to_array_debug($restored_obj))
    throw new Exception("unexpected !=");
}

function test_inheritance() {
  $obj = new C;
  $obj->c_int = 77;
  $obj->a_int = 44;

  $json = JsonEncoder::encode($obj);
  echo $json, "\n";
  $restored_obj = JsonEncoder::decode($json, "C");
  var_dump(to_array_debug($restored_obj));
}

class Chain {
  public ?Chain $next = null;
}

function test_chain_objects() {
  $obj = new Chain;
  $obj->next = new Chain;

  $json = JsonEncoder::encode($obj);
  echo $json, "\n";
  var_dump(to_array_debug(JsonEncoder::decode($json, Chain::class)));
}

function test_max_depth_level() {
  $obj = new Chain;
  $ptr = $obj;
  for ($i = 0; $i < 63; ++$i) {
    $ptr->next = new Chain;
    $ptr = $ptr->next;
  }

  // 64 lvl depth is ok
  $json = JsonEncoder::encode($obj);
  echo $json, "\n";
  var_dump(JsonEncoder::getLastError() == '');
  var_dump(JsonEncoder::decode($json, Chain::class) !== null);
  var_dump(JsonEncoder::getLastError() == '');

  $ptr->next = new Chain;

  // allowed depth exceeded
  var_dump(JsonEncoder::encode($obj));
  var_dump(JsonEncoder::getLastError());
}

function test_circular_reference() {
  $obj = new Chain;
  $obj->next = $obj;

  $json = JsonEncoder::encode($obj);
  var_dump($json);
  if ($json !== '')
    throw new Exception("unexpected json");
  var_dump(JsonEncoder::getLastError());
}

interface IValue {}

class SimpleValue implements IValue {
  public string $foo;
  function __construct() {
    $this->foo = 'bar';
  }
}

function test_interface() {
  $json = JsonEncoder::encode(new SimpleValue);
  var_dump($json);
  var_dump(to_array_debug(JsonEncoder::decode($json, SimpleValue::class)));
}

class Value implements IValue {
  public function toJson(): ?string {
    return JsonEncoder::encode($this);
  }

  public function debug(): void {
    var_dump(to_array_debug($this));
  }

  /** @return ?static */
  static public function fromJson(string $json) {
    return JsonEncoder::decode($json, static::class);
  }
}

class S1 extends Value {
  public int $s1 = 1;
}

class S2 extends Value {
  public int $s2 = 2;
}


function test_interface_regress() {
  echo (new S1)->toJson(), "\n";
  echo (new S2)->toJson(), "\n";
  S1::fromJson('{"s1":11}')->debug();
  S2::fromJson('{"s2":12}')->debug();
}


class ACirc {
  public ?BCirc $b = null;
}
class BCirc {
  public ?ACirc $a = null;
}

function test_circular_dep() {
  var_dump(JsonEncoder::encode(new ACirc));
  var_dump(JsonEncoder::encode(new BCirc));
}

class ACirc2 {
  /** @var BCirc2 */
  public $b;
}

class BCirc2 {
  /** @var ACirc2[] */
  public $a;
}

function test_circular_dep2() {
  $a = new ACirc2;
  $b = new BCirc2;
  $a->b = $b;
  $b->a[] = $a;

  var_dump(JsonEncoder::encode($a));
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::encode($b));
  var_dump(JsonEncoder::getLastError());
}

class ACirc3 {
  /** @var BCirc3[] */
  public $b;
}

class BCirc3 {
  /** @var ACirc3[] */
  public $a;
}

function test_circular_dep3() {
  $a = new ACirc3;
  $b = new BCirc3;
  $a->b[] = $b;
  $b->a[] = $a;

  var_dump(JsonEncoder::encode($a));
  var_dump(JsonEncoder::getLastError());

  var_dump(JsonEncoder::encode($b));
  var_dump(JsonEncoder::getLastError());
}

test_decode_nested_types();
test_decode_inheritance();
test_nested_types();
test_inheritance();
test_chain_objects();
test_max_depth_level();
test_circular_reference();
test_interface();
test_interface_regress();
test_circular_dep();
test_circular_dep2();
test_circular_dep3();
