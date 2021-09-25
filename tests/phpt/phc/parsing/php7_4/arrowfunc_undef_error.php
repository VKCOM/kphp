@kphp_should_fail
/Variable \$x1 may be used uninitialized/
/Variable \$x1 may be used uninitialized/
<?php

function test_undef() {
  // note that $x1 and $x2 are not defined in this lexical scope
  // but they are auto captured by fn(), converting to a call to L$xxx$$__construct(v$x1)
  // so, v$x1 in an outer function may be used uninitialized

  $f1 = fn() => ($x1 = 10) && $x1;
  var_dump($f1()); // => true

  $f2 = fn() => ($x2 = 0) && $x2;
  var_dump($f2()); // => false

  // if we var_dump($x1) in PHP, we'll see a notice also, because $x1 was not created in current scope
}

test_undef();
