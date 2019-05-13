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
          } else { # $x is A
              $x->_a();
          }
      } else { # $x is B
          $x->_b();
      }
  } else { # $x is IB
      $x->print_ib();
      if ($x instanceof C) {
          $x->_c();
      }
  }
}

/** @var IA $x */
$x = new A();
call_method($x);

$x = new B();
call_method($x);

$x = new C();
call_method($x);

