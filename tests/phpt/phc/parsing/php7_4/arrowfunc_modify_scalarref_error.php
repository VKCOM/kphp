@kphp_should_fail
/Modification of const variable: param/
<?php

function test_implicit_use_try_modify_scalarref(int &$param) {
  $f = fn() => $param = 23481;
  $result = $f();
  var_dump($param, $result);
}

$global_int = 730;
test_implicit_use_try_modify_scalarref($global_int);
var_dump($global_int);
