@ok
<?php

// Function parameters and the same variable were infered separately.
// function f(bool x) {x = new Memcached();}
// Everything will be ok in parser, but c++ ce error will happen.
// As a solution both xs may share TypeInf*
/*
function f($x) {
  $x = new Memcache();
}

f (true);
*/

function print_int ($x) {
  print $x;
}

function run_main_test_06() {
  $ints_array = array(1, 2, 3);
  foreach ($ints_array as $int_val) {
    print_int ($int_val);
  }
}
run_main_test_06();