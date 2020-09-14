@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @param int $x
 * @param int $p
 * @return float
 */
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
$res_powers = [];
while ($t = wait_queue_next ($q)) {
  $res_powers[] = wait_result ($t);
}
sort ($res_powers);
var_dump ($res_powers);

function a_varg(int $a, int $b, int $c = 7, int $d = 8) {
//  var_dump (func_get_args());
  var_dump ($a);
  var_dump ($b);
  var_dump ($c);
  var_dump ($d);
  b_varg(1, 2, 3);
}
/**
 * @kphp-infer
 * @param int ...$args
 */
function b_varg(...$args) {
  var_dump($args);
}

echo "-----------<stage 3>-----------\n";

a_varg(3, 2, 1);
$a = fork (a_varg(3, 2, 1));
wait_result ($a);

echo "-----------<stage 4>-----------\n";

$b = fork (b_varg(1, 2, 3));
wait_result ($b);
