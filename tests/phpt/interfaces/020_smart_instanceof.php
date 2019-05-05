@ok
<?php

interface IA {}

class A implements IA { public function _a() { var_dump("A"); } }
class B implements IA { public function _b() { var_dump("B"); } }
class C implements IA { public function _c() { var_dump("C"); } }

function call_method(IA $x) {
  if ($x instanceof A) {
      $x->_a();
  } else {
      var_dump("else");
      if ($x instanceof B) {
          $x->_b();
      } else {
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

