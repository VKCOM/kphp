@ok
<?php

require_once 'kphp_tester_include.php';

#ifndef KPHP
function preg_match_strings(string $re, string $s, int $offset = 0) {
  $m = [];
  $n = preg_match($re, $s, $m, 0, $offset);
  return tuple($n, $m);
}
#endif

// There are 3 main ways to call preg_match_strings:
//
//   (regexp, string)
//   (string, string)
//   (mixed, string)
//
// The second argument can also be a of a non-string type, but it can
// be implicitly converted to string without a problem.
// We have an extra test for mixed->string test, just in case.

/** @return mixed */
function string_to_mixed(string $s) { return $s; }

function test_regexp(string $s, int $offset) {
  [$n, $m] = preg_match_strings('/\d/', $s, $offset);
  var_dump($n, $m);

  $tuple = preg_match_strings('/\d/', $s, $offset);
  var_dump($tuple[0], $tuple[1]);

  /** @var mixed */
  $mixed_subject = string_to_mixed($s);
  $tuple = preg_match_strings('/\d/', $mixed_subject, $offset);
  var_dump($tuple[0], $tuple[1]);
}

function test_string(string $pat, string $s, int $offset) {
  [$n, $m] = preg_match_strings($pat, $s, $offset);
  var_dump($n, $m);

  $tuple = preg_match_strings($pat, $s, $offset);
  var_dump($tuple[0], $tuple[1]);

  /** @var mixed */
  $mixed_subject = string_to_mixed($s);
  $tuple = preg_match_strings($pat, $mixed_subject, $offset);
  var_dump($tuple[0], $tuple[1]);
}

/** @param mixed $pat */
function test_mixed($pat, string $s, int $offset) {
  [$n, $m] = preg_match_strings($pat, $s, $offset);
  var_dump($n, $m);

  $tuple = preg_match_strings($pat, $s, $offset);
  var_dump($tuple[0], $tuple[1]);

  /** @var mixed */
  $mixed_subject = string_to_mixed($s);
  $tuple = preg_match_strings($pat, $mixed_subject, $offset);
  var_dump($tuple[0], $tuple[1]);
}

$regexps = ['/\d+/', '/no matches/', '/^foo/', '/.*/', '/#?/'];
$inputs = ['430 43 1', '1', 'example', 'foo', '-40', 'hello world', 'longer string with numbers (43) and # symbol'];
$offsets = [0, 1, 3, 10, 30, 100, -1, -3, -100];

foreach ($regexps as $re) {
  foreach ($inputs as $s) {
    foreach ($offsets as $offset) {
      test_regexp($s, $offset);
      test_string($re, $s, $offset);
      test_mixed($re, $s, $offset);
    }
  }
}

