@ok
<?php

use function \A\B\C\f as f2;

require_once __DIR__ . '/lib1.php';

function f() {
  var_dump(__FUNCTION__);
}

function test() {
  f();
  var_dump(\A\B\C\f());
  var_dump(f2());
}
