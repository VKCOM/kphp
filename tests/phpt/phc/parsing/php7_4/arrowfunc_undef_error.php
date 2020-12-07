@kphp_should_fail
/Modification of const variable: x1/
/Modification of const variable: x2/
<?php

function test_undef() {
  // note that $x1 and $x2 are not defined in this lexical scope

  $f1 = fn() => ($x1 = 10) && $x1;
  var_dump($f1()); // => true

  $f2 = fn() => ($x2 = 0) && $x2;
  var_dump($f2()); // => false

  // $x1 is not defined in this scope
  // $x2 is not defined in this scope
}

test_undef();
