@ok
<?php

/** @param string $x */
function ensure_string($x) {}

/** @param string[] $x */
function ensure_string_array($x) {}

function test_without_flags_and_offset(string $re, string $s) {
  var_dump(preg_match_all($re, $s, $m1));
  var_dump(preg_last_error());
  var_dump($m1);
  ensure_string_array($m1[0]);
  ensure_string($m1[0][0]);

  $m2 = [];
  var_dump(preg_match_all($re, $s, $m2));
  var_dump(preg_last_error());
  var_dump($m2);
  ensure_string_array($m2[0]);
  ensure_string($m2[0][0]);

  /** @var mixed $m3 */
  $m3 = [];
  var_dump(preg_match_all($re, $s, $m3));
  var_dump(preg_last_error());
  var_dump($m3);
}

function test_const_regexp_without_flags_and_offset(string $re, string $s) {
  var_dump(preg_match_all('/\d/', $s, $m1));
  var_dump(preg_last_error());
  var_dump($m1);
  ensure_string_array($m1[0]);
  ensure_string($m1[0][0]);

  $m2 = [];
  var_dump(preg_match_all('/\d/', $s, $m2));
  var_dump(preg_last_error());
  var_dump($m2);
  ensure_string_array($m2[0]);
  ensure_string($m2[0][0]);

  /** @var mixed $m3 */
  $m3 = [];
  var_dump(preg_match_all('/\d/', $s, $m3));
  var_dump(preg_last_error());
  var_dump($m3);
}

$regexps = ['/\d+/', '/no matches/', '/^foo/', '/.*/', '/#?/'];
$inputs = [
  '430 43 1 ',
  '1',
  'example',
  ' foo ',
  'foo _foo_',
  '-40',
  'hello world',
  'longer string with numbers (43) and # symbol',
];

foreach ($regexps as $re) {
  foreach ($inputs as $s) {
    test_without_flags_and_offset($re, $s);
    test_const_regexp_without_flags_and_offset($re, $s);
  }
}
