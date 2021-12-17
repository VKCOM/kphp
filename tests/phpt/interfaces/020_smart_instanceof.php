@ok
<?php

interface I { public function print_i(); }

interface IA extends I { public function print_ia(); }
interface IB extends I { public function print_ib(); }

class A implements IA {
    public function print_i() { var_dump("print_i::A"); }
    public function print_ia() { var_dump("print_ia::A"); }

    public function _a() { var_dump("_a"); }
}

class B implements IA {
    public function print_i() { var_dump("print_i::B"); }
    public function print_ia() { var_dump("print_ia::B"); }

    public function _b() { var_dump("_b"); }
}

class C implements IB {
    public function print_i() { var_dump("print_i::C"); }
    public function print_ib() { var_dump("print_ib::C"); }

    public function _c() {
        var_dump("_c");
    }
}

function call_method(I $x) {
  $x->print_i();
  if ($x instanceof IA) {
      $x->print_ia();
      if ($x instanceof A) {
          $x->_a();
          if ($x instanceof B) { # just for fun
              $x->_b();
          } else if ($x instanceof A) {
              $x->_a();
          }
      } else if ($x instanceof B) {
          $x->_b();
      }
  } else if ($x instanceof IB) {
      $x->print_ib();
      if ($x instanceof C) {
          $x->_c();
      }
  }
}

/** @var I $x */
$x = new A();
call_method($x);

$x = new B();
call_method($x);

$x = new C();
call_method($x);



interface I2 { function f(); }

class A2 implements I2 {
    public $field = 0;
    function f() { echo "A f {$this->field}\n"; }
    function __construct(int $f = 0) { $this->field = $f; }
}

class B2 implements I2 {
    function f() { echo "B f\n"; }
}

function getField(?I2 $i): int {
    if ($i)
        $i->f();
    if (!($i instanceof A2))
        return -1;
    return $i->field;
}

function test_not_instanceof_1() {
    echo getField(new A2), "\n";
    echo getField(new A2(-5)), "\n";
    echo getField(null), "\n";
}

function test_not_instanceof_2() {
    /** @var ?I2 $a */
    $a = new A2;
    if (!($a instanceof A2))
        return;
    if ($a instanceof A2)
        $a->f();
}

interface IResult {}
class CSuccess implements IResult { public $code = 123; }
class CError implements IResult { public $err = 'err'; }

function test_not_instanceof_3(): int {
    /** @var IResult $result */

    if (0) $result = new CSuccess;
    else $result = new CError;
    if (!($result instanceof CSuccess)) {
        if ($result instanceof CError)
            echo $result->err, "\n";
    }

    if (1) $result = new CSuccess;
    else $result = new CError;
    if (!($result instanceof CError))
        echo -100, "\n";
    if ($result instanceof CSuccess)
        echo $result->code, "\n";

    /** @var IResult $r2 */
    $r2 = new CSuccess;
    if (0) $r2 = new CError;

    if (!($r2 instanceof CSuccess))
        return -100;
    if (!($r2 instanceof CSuccess))
        return -200;
    if (!($result instanceof CError))
        return $r2->code;
    return -500;
}

function test_not_instanceof_4(): string {
    /** @var I2 $i */
    $i = new A2;
    if (!($i instanceof B2))
        return get_class($i);
    return get_class($i);
}

test_not_instanceof_1();
test_not_instanceof_2();
var_dump(test_not_instanceof_3());
var_dump(test_not_instanceof_4());
