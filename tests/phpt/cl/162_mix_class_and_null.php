@ok k2_skip
<?php

class A {
  public $x = 1;

  /**
   * @param int $x
   */
  public function __construct($x = 1) {
    $this->x = $x;
  }
}

class B {
  /** @var A */
  public $a1 = null;
  /** @var A|null */
  public $a2 = null;
}

/**
 * @return A[]
 */
function test_array_mixing() {
  $x = [new A, null];
  var_dump(isset($x[0]));
  var_dump(isset($x[1]));
  var_dump(isset($x[2]));
  return $x;
}

function test_class_fields() {
  $b = new B;

  $b->a1 = new A(5);
  echo $b->a1 ? $b->a1->x : "0", "\n";
  echo $b->a2 ? $b->a2->x : "0", "\n";
  $b->a2 = new A(6);
  echo $b->a1 ? $b->a1->x : "0", "\n";
  echo $b->a2 ? $b->a2->x : "0", "\n";
  $b->a2 = null;
  echo $b->a1 ? $b->a1->x : "0", "\n";
  echo $b->a2 ? $b->a2->x : "0", "\n";
}

/**
 * @param A $a
 */
function accepts_instance_1($a) {
  echo $a ? $a->x : "0", "\n";
}

/**
 * @param A|null $a
 */
function accepts_instance_2($a) {
  echo $a ? $a->x : "0", "\n";
}

function accepts_instance_3(A $a = null) {
  echo $a ? $a->x : "0", "\n";
}

/**
 * @return A
 */
/**
 * @param int $init_v
 * @return A
 */
function returns_instance($init_v) {
  return $init_v ? new A($init_v) : null;
}

/**
 * @return A|null
 */
/**
 * @param int $init_v
 * @return A
 */
function returns_instance_2($init_v) {
  return $init_v ? new A($init_v) : null;
}

$arr = test_array_mixing();
echo count($arr), "\n";

test_class_fields();

accepts_instance_1(new A(7));
accepts_instance_1($arr[0]);
accepts_instance_1($arr[100500]);
accepts_instance_1(null);

accepts_instance_2(new A(8));
accepts_instance_2($arr[0]);
accepts_instance_2($arr[100500]);
accepts_instance_2(null);

accepts_instance_3(new A(8));
accepts_instance_3($arr[0]);
accepts_instance_3($arr[100500]);
accepts_instance_3(null);

accepts_instance_2(returns_instance(1));
accepts_instance_2(returns_instance(0));
accepts_instance_2(returns_instance_2(1));
accepts_instance_2(returns_instance_2(0));

/**
 * @param RpcConnection $c
 */
function accepts_rpc_connection($c) {
  echo $c ? "has conn" : "no conn", "\n";
}

accepts_rpc_connection(new_rpc_connection('127.0.0.1', 127001));
accepts_rpc_connection(null);
$c = new_rpc_connection('127.0.0.1', 127001);
accepts_rpc_connection($c);
$c = null;
accepts_rpc_connection($c);

$a = new A;
if(0) $a = null;
echo $a ? $a->x : " ", "\n";
if(1) $a = returns_instance(0);
echo $a ? $a->x : " ", "\n";

$e = new Exception();
if(0) $e = null;
echo $e ? basename($e->getFile()) : "", "\n";
if(1) $e = null;
echo $e ? basename($e->getFile()) : "", "\n";

