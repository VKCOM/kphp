@kphp_should_fail
/Modification of const variable: param/
<?php

function test_implicit_use_try_modify_arrayref(array &$param) {
  $f = fn() => $param[] = 10;
  $result = $f();
  var_dump($param, $result);
}

$global_array = [1, 2];
test_implicit_use_try_modify_arrayref($global_array);
var_dump($global_array);
