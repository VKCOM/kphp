@kphp_should_fail
<?php

class A {
  public $x = 1;
}

function test_class_mixing() {
  $x = [new A, null];
  return $x;
}

test_class_mixing();
