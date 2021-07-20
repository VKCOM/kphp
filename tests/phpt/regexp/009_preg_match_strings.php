@ok
<?php

/** @param string[] $x */
function ensure_strings($x) {}

function test_without_flags_and_offset(string $re, string $s) {
  var_dump(preg_match($re, $s, $m1));
  var_dump(preg_last_error());
  var_dump($m1);
  ensure_strings($m1);

  $m2 = [];
  var_dump(preg_match($re, $s, $m2));
  var_dump(preg_last_error());
  var_dump($m2);
  ensure_strings($m2);

  /** @var mixed $m3 */
  $m3 = [];
  var_dump(preg_match($re, $s, $m3));
  var_dump(preg_last_error());
  var_dump($m3);
}

function test_with_flags_and_offset(string $re, string $s, int $offset) {
  var_dump(preg_match($re, $s, $m1, 0, $offset));
  var_dump(preg_last_error());
  var_dump($m1);
  ensure_strings($m1);

  $m2 = [];
  var_dump(preg_match($re, $s, $m2, 0, $offset));
  var_dump(preg_last_error());
  var_dump($m2);
  ensure_strings($m2);

  /** @var mixed $m3 */
  $m3 = [];
  var_dump(preg_match($re, $s, $m3, 0, $offset));
  var_dump(preg_last_error());
  var_dump($m3);
}

function test_const_regexp_without_offset(string $s) {
  var_dump(preg_match('/\d/', $s, $m));
  var_dump($m);

  /** @var mixed $m2 */
  $m2 = [];
  var_dump(preg_match('/\d/', $s, $m2));
  var_dump($m2);
}

function test_const_regexp_with_flags_and_offset(string $s, int $offset) {
  var_dump(preg_match('/\d/', $s, $m, 0, $offset));
  var_dump($m);

  /** @var mixed $m2 */
  $m2 = [];
  var_dump(preg_match('/\d/', $s, $m2, 0, $offset));
  var_dump($m2);
}

$regexps = ['/\d+/', '/no matches/', '/^foo/', '/.*/', '/#?/'];
$inputs = ['430 43 1', '1', 'example', 'foo', '-40', 'hello world', 'longer string with numbers (43) and # symbol'];
$offsets = [0, 1, 3, 10, 30, 100, -1, -3, -100];

foreach ($regexps as $re) {
  foreach ($inputs as $s) {
    foreach ($offsets as $offset) {
      test_without_flags_and_offset($re, $s);
      test_with_flags_and_offset($re, $s, $offset);
      test_const_regexp_without_offset($s);
      test_const_regexp_with_flags_and_offset($s, $offset);
    }
  }
}
