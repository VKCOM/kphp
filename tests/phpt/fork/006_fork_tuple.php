@ok
<?php

require_once 'kphp_tester_include.php';

/**
 * @return tuple(int, int, string, any[])
 */
function return_tuple1() {
  sched_yield();
  return tuple(1, 2, "", []);
}

/**
 * @return tuple(int, int, int[], mixed[])
 */
function return_tuple2() {
  sched_yield();
  return tuple(1, 2, [1, 1], [1, ""]);
}

/**
 * @param tuple(int, int, mixed, mixed[])|null $x
 * @return mixed[]
 */
function to_array($x) {
  return [$x[0], $x[1], $x[2], $x[3]];
}

var_dump(to_array(wait(fork(return_tuple1()))));
var_dump(to_array(wait(fork(return_tuple2()))));

$q = [fork(return_tuple1()), fork(return_tuple2())];
var_dump(to_array(wait($q[0])));
var_dump(to_array(wait($q[1])));
