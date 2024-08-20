@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function ret_string_or_null(int $line, ?string $res) : ?string {
  echo "call ret_string_or_null:$line\n";
  sched_yield();
  return $res;
}

function test_lhs() {
  $x = ret_string_or_null(__LINE__, "123") ?? "456";
  var_dump($x);

  $x = ret_string_or_null(__LINE__, null) ?? "xxxxx";
  var_dump($x);
  return null;
}

function test_in_if() {
  if ($x = ret_string_or_null(__LINE__, "123") ?? "NULLLL") {
    echo "if body\n";
    var_dump($x);
  } else {
    echo "else body\n";
    var_dump($x);
  }
  return null;
}

wait(fork(test_lhs()));
wait(fork(test_in_if()));
