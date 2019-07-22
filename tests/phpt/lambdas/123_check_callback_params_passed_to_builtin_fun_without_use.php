@kphp_should_fail
<?php

function f() {
  $y = array_map(function() { return 20;}, [1, 2, 3]);
  return $y;
}
var_dump(f());
