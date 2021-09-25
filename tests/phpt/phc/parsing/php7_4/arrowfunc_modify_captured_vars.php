@ok
<?php

function test_implicit_use_try_modify_scalar(int $param) {
  $f = fn() => $param = 23481;
  $result = $f();
  var_dump($param, $result);
}

test_implicit_use_try_modify_scalar(43);


function test_implicit_use_try_modify_scalarref(int &$param) {
  $f = fn() => $param = 23481;
  $result = $f();
  var_dump($param, $result);
}

$global_int = 730;
test_implicit_use_try_modify_scalarref($global_int);
var_dump($global_int);


function test_implicit_use_try_modify_arrayref(array &$param) {
  $f = fn() => $param[] = 10;
  $result = $f();
  var_dump($param, $result);
}

$global_array = [1, 2];
test_implicit_use_try_modify_arrayref($global_array);
var_dump($global_array);



