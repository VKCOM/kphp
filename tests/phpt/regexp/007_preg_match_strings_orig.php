@ok
<?php

// Make sure that we don't replace the non-zero flag forms

function test_without_flags_and_offset(string $re, string $s) {
  var_dump(preg_match($re, $s, $m1, PREG_OFFSET_CAPTURE));
  var_dump(preg_last_error());
  var_dump($m1);

  $m2 = [];
  var_dump(preg_match($re, $s, $m2, PREG_OFFSET_CAPTURE));
  var_dump(preg_last_error());
  var_dump($m2);

  /** @var mixed $m3 */
  $m3 = [];
  var_dump(preg_match($re, $s, $m3, PREG_OFFSET_CAPTURE));
  var_dump(preg_last_error());
  var_dump($m3);
}

function test_with_flags_and_offset(string $re, string $s, int $offset) {
  var_dump(preg_match($re, $s, $m1, PREG_OFFSET_CAPTURE, $offset));
  var_dump(preg_last_error());
  var_dump($m1);

  $m2 = [];
  var_dump(preg_match($re, $s, $m2, PREG_OFFSET_CAPTURE, $offset));
  var_dump(preg_last_error());
  var_dump($m2);

  /** @var mixed $m3 */
  $m3 = [];
  var_dump(preg_match($re, $s, $m3, PREG_OFFSET_CAPTURE, $offset));
  var_dump(preg_last_error());
  var_dump($m3);
}

$regexps = ['/\d+/', '/no matches/', '/^foo/', '/.*/', '/#?/'];
$inputs = ['430 43 1', '1', 'example', 'foo', '-40', 'hello world', 'longer string with numbers (43) and # symbol'];
$offsets = [0, 1, 3, 10, 30, 100, -1, -3, -100];

foreach ($regexps as $re) {
  foreach ($inputs as $s) {
    foreach ($offsets as $offset) {
      test_without_flags_and_offset($re, $s);
      test_with_flags_and_offset($re, $s, $offset);
    }
  }
}
