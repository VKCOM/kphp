@ok k2_skip
<?php
/**
 * @param any[] $g
 */
function g($g) {
  var_dump($g);
}

g(array());


function f($x, $z, ...$args) {
  var_dump($x);
  var_dump($z);

  array_unshift($args, $z);
  array_unshift($args, $x);
  var_dump($args);

  $n = count($args);
  var_dump ($n);

  for ($i = 0; $i < $n; $i++) {
    var_dump ($args[$i]);
  }

  if (1) {
    $tmp = $z;
  }
  if (1) {
    $tmp = 1;
  }
}
f(1, 2, 3, "hello", [1=>23]);
f(1, 2);
f(1, 2);
f(1, 2, 3, "hello", [1=>23]);
f("hello", 1);
//call_user_func_array ("g", 123);

