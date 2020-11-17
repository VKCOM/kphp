@kphp_should_fail
/Modification of const variable: param/
<?php

function test_implicit_use_try_modify_scalar(int $param) {
  $f = fn() => $param = 23481;
  $result = $f();
  var_dump($param, $result);
}

test_implicit_use_try_modify_scalar(43);
