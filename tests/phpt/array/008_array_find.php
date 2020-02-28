@ok
<?php
require_once __DIR__ . '/../polyfills.php';

function run_test() {
  $ar = [1,2,3,4,4,5,6,6];
  [$key, $value] = array_find($ar, function($v) { return $v > 5;});
  var_dump($key, $value);

  $ar = [1, false, "something"];
  [$key, $value] = array_find($ar, 'is_bool');
  var_dump($key, $value);

  $ar = [1, false, "something"];
  [$key, $value] = array_find($ar, 'is_float');
  var_dump($key, $value);

  $ar = [1,2,3,4,6];
  [$key, $value] = array_find($ar, function($v) { return $v > 1000;});
#ifndef KittenPHP
  $value = 0;
#endif
  var_dump($key, $value);

  $ar = ['a', 'b', 'c', 'd'];
  [$key, $value] = array_find($ar, function($v) { return $v === 'e';});
#ifndef KittenPHP
  $value = '';
#endif
  var_dump($key, $value);

  $ar = [];
  if (time() < 0) {
    $ar = ['1', 1, 3, 4];
  }
  [$key, $value] = array_find($ar, function($v) { return $v === 11;});
  var_dump($key, $value);
}
run_test();

