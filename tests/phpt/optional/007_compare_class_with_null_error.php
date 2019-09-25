@kphp_should_fail
/isset\(\$x\) will be always true \(inferred A\)/
<?php

class A {
  public $x = 1;
}

function test_class_compare(A $x) {
  return $x === null;
}

test_class_compare(new A);
