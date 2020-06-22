@kphp_should_fail
/result of operation is both B and A/
<?php

class A {
  public $x = 1;
}

class B {
  public $x = 1;
}

function test(A $a, ?B $b) {
  $v = $b ? $b : $a;
  var_dump($v->x);
}

test(new A, new B);
