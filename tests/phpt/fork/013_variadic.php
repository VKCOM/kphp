@ok
<?php
require_once __DIR__ . "/../dl/polyfill/fork-php-polyfill.php";

function my_pow ($x, $p = 5) {
  if ($p == 0) {
    return 1.0;
  }
  $tmp = my_pow ($x, $p - 1);
  return $tmp * $x;
}

$x = fork (my_pow (3, 2));
$y = fork (my_pow (5, 3));
$z = fork (my_pow (2));

echo "-----------<stage 1>-----------\n";

var_dump (wait ($x));
var_dump (wait ($y));
var_dump (wait ($z));
var_dump (wait_result ($y));
var_dump (wait_result ($y));
var_dump (rpc_get ($x));
var_dump (wait_result ($x));
var_dump (wait_result ($z));

$x = fork (my_pow (3, 2));
$y = fork (my_pow (5, 3));
$z = fork (my_pow (2));

echo "-----------<stage 2>-----------\n";

$q = wait_queue_create (array ($x, $y, $z));
while ($t = wait_queue_next ($q)) {
  var_dump (wait_result ($t));
}

function a_varg($a, $b, $c = 7, $d = 8) {
  var_dump (func_get_args());
  var_dump ($a);
  var_dump ($b);
  var_dump ($c);
  var_dump ($d);
  b_varg(1, 2, 3);
}
function b_varg() {
  var_dump (func_get_args());
}

echo "-----------<stage 3>-----------\n";

a_varg(3, 2, 1);
$a = fork (a_varg(3, 2, 1));
wait_result ($a);

echo "-----------<stage 4>-----------\n";

$b = fork (b_varg(1, 2, 3));
wait_result ($b);
