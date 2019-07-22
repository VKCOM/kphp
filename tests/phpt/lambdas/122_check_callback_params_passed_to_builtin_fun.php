@kphp_should_fail
<?php

function f() {
  $x = 20;
  $y = array_map(function() use ($x) { return 20;}, [1, 2, 3]);
  return $y;
}
var_dump(f());
