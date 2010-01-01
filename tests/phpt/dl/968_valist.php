@ok
<?php
function g($g) {
  var_dump($g);
}

g(array());


function f($x, $z = 12) {
  var_dump ($x);
  var_dump ($z);

  $args = func_get_args();
  var_dump($args);

  $n = func_num_args();
  var_dump ($n);

  for ($i = 0; $i < $n; $i++) {
    var_dump (func_get_arg ($i));
  }

  if (1) {
    $tmp = $z;
  }
  if (1) {
    $tmp = 1;
  }
}
f(1, 2, 3, "hello", array (1=>23));
f(1);
f(1, 2);

call_user_func_array ("f", array (1, 2, 3, "hello", array (1=>23)));
call_user_func_array ("f", array ("hello", 1));
//call_user_func_array ("g", 123);

