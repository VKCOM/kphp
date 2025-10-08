<?php

function main() {
  switch ($_SERVER["PHP_SELF"]) {
    case "/store": {
      test_store();
      return;
    }
    case "/fetch_and_verify": {
      test_fetch_and_verify();
      return;
    }
    case "/delete": {
      test_delete();
      return;
    }
  }

  critical_error("unknown test " . $_SERVER["PHP_SELF"]);
}


/** @kphp-immutable-class
 *  @kphp-serializable */
class TestClassA {
  function __construct(TestClassA $a1 = null, TestClassA $a2 = null,
                       TestClassB $b1 = null, TestClassB $b2 = null,
                       TestClassC $c1 = null, TestClassC $c2 = null) {
    if ($a1 === null) {
      for ($i = 0; $i != 3000; ++$i) {
        $this->huge_arr["key $i"] = "value $i";
      }
      return;
    }
    $this->a = [$a1, $a2, $a2, $this, $a1, $this, $a1];
    for ($i = 0; $i != 10; ++$i) {
      array_push($this->a, $a1);
      array_push($this->a, $a2);
      array_push($this->a, $this);
    }
    $this->b = tuple([$b1, $b2, $b1, $b1, $b2], 100);
// shape is not serializable. So it is temporary commented
//     $this->ab = shape([
//       'b' => tuple(10, $b2),
//       'c' => shape([
//         'arr' => [$c1, $c1, $c2, $c2, $c2, $c2]
//       ])
//     ]);
  }

  /** @var TestClassA[]
   *  @kphp-serialized-field 0 */
  public $a = [];

  /** @var tuple(TestClassB[]|null, int)
   *  @kphp-serialized-field 1 */
  public $b;

  /** @var string[]
   *  @kphp-serialized-field 3 */
  public $huge_arr = [];
}

/** @kphp-immutable-class
 *  @kphp-serializable */
class TestClassB {
  function __construct(int $size, TestClassA $a = null) {
    while (--$size > 0) {
      $this->arr["A key $size"] = "A value $size";
    }
    if ($a) {
      $this->a = tuple(TestClassA::class, $a);
    }
  }

  /** @var string[]
   *  @kphp-serialized-field 0 */
  public $arr = [];

  /** @var tuple(string, TestClassA)|null
   *  @kphp-serialized-field 1 */
  public $a = null;
}

/** @kphp-immutable-class
 *  @kphp-serializable */
class TestClassC {
  function __construct(int $size, TestClassB $b = null) {
    while (--$size > 0) {
      $this->arr["B key $size"] = "B value $size";
    }

    if ($b) {
//       $this->b = shape([
//         'key' => TestClassB::class,
//         'value' => tuple(0.1, [$b, $b, $b, $b])
//       ]);
    }
  }

  /** @var string[]
   *  @kphp-serialized-field 0 */
  public $arr = [];

//   /** @var shape(key:string, value:tuple(double, TestClassB[]|null)|false)|null */
//   public $b = null;
}

/** @kphp-immutable-class
 *  @kphp-serializable */
class TestClassABC {
  function __construct() {
    $a1 = new TestClassA();
    $b1 = new TestClassB(10, $a1);
    $c1 = new TestClassC(11, $b1);

    $a2 = new TestClassA(
      $a1, new TestClassA(),
      $b1, new TestClassB(12),
      $c1, new TestClassC(13));

    $this->a = [$a1, $a2, new TestClassA($a1, $a2)];
    $this->b = [$b1, new TestClassB(14, $a2)];
    $this->c = [$c1, new TestClassC(15, $this->b[1])];
  }

  /** @var TestClassA[]
   *  @kphp-serialized-field 0 */
  public $a = [];
  /** @var TestClassB[]
   *  @kphp-serialized-field 1 */
  public $b = [];
  /** @var TestClassC[]
   *  @kphp-serialized-field 2 */
  public $c = [];
}

function test_store() {
  $data = json_decode(file_get_contents('php://input'));
  echo json_encode(["result" => instance_cache_store((string)$data["key"], new TestClassABC, 5)]);
}

function test_fetch_and_verify() {
  $data = json_decode(file_get_contents('php://input'));
  /** @var TestClassABC $instance */
  $instance = instance_cache_fetch(TestClassABC::class, (string)$data["key"]);
  echo json_encode([
    "a" => $instance->a[0] === $instance->a[2]->a[0],
    "b" => $instance->b[0] === $instance->b[1]->a[1]->b[0][0],
//     "c" => $instance->c[0] === $instance->a[2]->a[1]->ab["c"]["arr"][1],
  ]);
}

function test_delete() {
  $data = json_decode(file_get_contents('php://input'));
  instance_cache_delete((string)$data["key"]);
}

main();
