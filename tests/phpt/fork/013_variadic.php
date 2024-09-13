@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/**
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

var_dump (wait($x) != null);
var_dump (wait($y) != null);
var_dump (wait($z) != null);
var_dump (wait($y));
var_dump (wait($y));
var_dump (rpc_get ($x));
var_dump (wait($x));
var_dump (wait($z));

$x = fork (my_pow (3, 2));
$y = fork (my_pow (5, 3));
$z = fork (my_pow (2));

echo "-----------<stage 2>-----------\n";

$q = wait_queue_create (array ($x, $y, $z));
$res_powers = [];
while ($t = wait_queue_next ($q)) {
  $res_powers[] = wait($t);
}
sort ($res_powers);
var_dump ($res_powers);

function a_varg(int $a, int $b, int $c = 7, int $d = 8) {
//  var_dump (func_get_args());
  var_dump ($a);
  var_dump ($b);
  var_dump ($c);
  var_dump ($d);
  return b_varg(1, 2, 3);
}
/**
 * @param int ...$args
 */
function b_varg(...$args) {
  var_dump($args);
  return $args;
}

echo "-----------<stage 3>-----------\n";

a_varg(3, 2, 1);
$a = fork (a_varg(3, 2, 1));
wait($a);

echo "-----------<stage 4>-----------\n";

$b = fork (b_varg(1, 2, 3));
wait($b);
