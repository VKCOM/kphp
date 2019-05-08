@kphp_should_fail
<?php

interface IA {}

class A implements IA { public function _a() { var_dump("A"); } }
class B implements IA { public function _b() { var_dump("B"); } }

function call_method(IA $x) {
  if ($x instanceof A) {
      $x = new A();
  }
}

/** @var IA $x */
$x = new B();
$x = new A();
call_method($x);

