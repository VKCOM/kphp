@ok no_php
<?
//function f() {
  //g();
  //h();
//}
//function g() {
  //global $x;
  //$x[1] = 2;
//}
//function h() {
  //resumable_f();
//}

//function resumable_f() resumable {
//}

//fork (f());

//function tmp_f() resumable {
//}

//foreach ($x as &$y) {
  ////produce "dangerous undefined behaviour warning"
  //tmp_f();
//}

function my_pow ($x, $p = 5) resumable {
  if ($p == 0) {
    return 1.0;
  }
  $tmp = my_pow ($x, $p - 1);
  return $tmp * $x;
}

$x = fork (my_pow (3, 2));
$y = fork (my_pow (5, 3));
$z = fork (my_pow (2));

var_dump (wait ($x));
var_dump (wait ($x + 1));
var_dump (wait ($x + 2));
var_dump (wait_result ($y));
var_dump (wait_result ($y));
var_dump (rpc_get ($x));
var_dump (wait_result ($x));
var_dump (wait_result ($z));

$x = fork (my_pow (3, 2));
$y = fork (my_pow (5, 3));
$z = fork (my_pow (2));

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
a_varg(3, 2, 1);
$a = fork (a_varg(3, 2, 1));
wait_result ($a);
$b = fork (b_varg(1, 2, 3));
wait_result ($b);
