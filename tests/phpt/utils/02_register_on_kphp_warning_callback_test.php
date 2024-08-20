@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function make_warning() {
  $a = 0 / 0;
  $a += 0;
}

function basic_test() {
  $callback = function($message, $stacktrace) {
    echo "This is some kind of warning\n";
  };
  register_kphp_on_warning_callback($callback);
  make_warning();
}

$recursion = false;

function endless_recursion_test() {
  register_kphp_on_warning_callback(function($message, $stacktrace) {
    global $recursion;
    if ($recursion) {
      echo "FAIL! endless recursion\n";
    }
    $recursion = true;
    make_warning();
  });
  make_warning();
}

function inner_register_calls_test() {
  $arr = [1, 2, 3];
  $arr[] = 4;
  register_kphp_on_warning_callback(
    function($message, $stacktrace) use ($arr) {
      register_kphp_on_warning_callback(function($str, $tr) {});
      $x = $arr[3] + 1;
      $x += 0;
    }
  );
  make_warning();
}

basic_test();
endless_recursion_test();
inner_register_calls_test();
