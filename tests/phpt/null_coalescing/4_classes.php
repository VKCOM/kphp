@ok k2_skip
<?php

class A {
  /**
   * @param int $x
   */
  public function __construct($x) {
    echo "call A::__construct($x)\n";
    $this->x = $x;
  }

  public $x = 123;
}

class B {
  /**
   * @param A $a
   * @param int $x
   */
  public function __construct($a, $x) {
    echo "call B::__construct($x)\n";
    $this->a = $a;
  }

  /** @var A */
  public $a = null;
}


class C {
  /**
   * @param int $x
   */
  public function __construct($x) {
    echo "call C::__construct($x)\n";
  }
}

function test_classes_variable() {
  $a0 = 0 ? new A(1) : null;
  $a1 = new A(2);

  /** @var A */
  $x0 = $a0 ?? new A(3);
  /** @var A */
  $x1 = $a0 ?? $a1;
  /** @var A */
  $x2 = $a1 ?? $a0;
  /** @var A */
  $x3 = $a1 ?? new A(4);
  /** @var A */
  $x4 = $a0 ?? $a1 ?? new A(5);
  /** @var A */
  $x5 = new A(6) ?? new A(7) ?? new A(8);

  var_dump($x0->x);
  var_dump($x1->x);
  var_dump($x2->x);
  var_dump($x3->x);
  var_dump($x4->x);
  var_dump($x5->x);
}

function test_empty_class() {
  $c0 = 0 ? new C(1) : null;
  $c1 = new C(2);

  $x0 = $c0 ?? new C(3);
  $x1 = $c0 ?? $c1;
  $x2 = $c1 ?? $c0;
  $x3 = $c1 ?? new C(4);
  $x4 = $c0 ?? $c1 ?? new C(5);

  var_dump(!$x0);
  var_dump(!$x1);
  var_dump(!$x2);
  var_dump(!$x3);
  var_dump(!$x4);
}

function test_class_field() {
  $b0 = new B(null, 1);
  $b1 = new B(new A(6), 1);

  /** @var A */
  $x0 = $b0->a ?? new A(7);
  /** @var A */
  $x1 = $b1->a ?? new A(8);
  /** @var A */
  $x2 = $b1->a ?? $b0->a;
  /** @var A */
  $x3 = $b0->a ?? $b1->a;
  /** @var A */
  $x4 = $b0->a ?? null ?? $b1->a ?? null;
  /** @var A */
  $x5 = $b0->a ?? null ?? new A(9);

  var_dump($x0->x);
  var_dump($x1->x);
  var_dump($x2->x);
  var_dump($x3->x);
  var_dump($x4->x);
  var_dump($x5->x);
}

function test_class_array() {
  $arr = [];
  $arr[] = new A(1);
  $arr[] = new A(2);
  $arr[] = null;
  $arr[] = new A(4);

  /** @var A */
  $x0 = $arr[10] ?? new A(5);
  /** @var A */
  $x1 = $arr[1] ?? new A(6);
  /** @var A */
  $x2 = $arr[10] ?? $arr[1];
  /** @var A */
  $x3 = $arr[2] ?? $arr[0];
  /** @var A */
  $x4 = $arr[2] ?? new A(7);
  /** @var A */
  $x5 = $arr[10] ?? $arr[2] ?? new A(8);

  var_dump($x0->x);
  var_dump($x1->x);
  var_dump($x2->x);
  var_dump($x3->x);
  var_dump($x4->x);
  var_dump($x5->x);
}

function test_or_false_or_null_with_class_and_rpc_connection() {
  $y = 1 ? null : new A(1);

  $a = $y ?? new A(1);
  var_dump(!$a);

  $x = 1 ? null : new_rpc_connection("", 0);
  $rpc_conn = $x ?? new_rpc_connection("", 0);
  var_dump($rpc_conn === null);
}

test_classes_variable();
test_empty_class();
test_class_field();
test_class_array();
test_or_false_or_null_with_class_and_rpc_connection();