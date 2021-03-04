@ok
<?php

#ifndef KPHP
function preg_match_strings(string $re, string $s, &$m, int $offset = 0) {
  return preg_match($re, $s, $m, 0, $offset);
}
#endif

// There are 12 main ways to call preg_match_strings:
//
//   (regexp, string, array<mixed>)
//   (regexp, string, array<string>)
//   (string, string, array<mixed>)
//   (string, string, array<string>)
//   (mixed, string, array<mixed>)
//   (mixed, string, array<string>)
//   (regexp, string, array<mixed>, int)
//   (regexp, string, array<string>, int)
//   (string, string, array<mixed>, int)
//   (string, string, array<string>, int)
//   (mixed, string, array<mixed>, int)
//   (mixed, string, array<string>, int)
//
// The second argument can also be a of a non-string type, but it can
// be implicitly converted to string without a problem.
// We have an extra test for mixed->string test, just in case.

function test_re_string_mixed(string $s) {
  /** @var mixed $m1 */
  $m1 = [];
  var_dump(preg_match_strings('/\d+/', $s, $m1));
  var_dump($m1);

  /** @var array $m2 */
  $m2 = [];
  var_dump(preg_match_strings('/no matches/', $s, $m2));
  var_dump($m2);

  /** @var mixed[] $m2 */
  $m3 = [];
  var_dump(preg_match_strings('/\d+/', $s, $m3));
  var_dump($m3);
}

function test_re_string_mixed_offset(string $s, int $offset) {
  /** @var mixed $m1 */
  $m1 = [];
  var_dump(preg_match_strings('/\d+/', $s, $m1));
  var_dump($m1);

  /** @var array $m2 */
  $m2 = [];
  var_dump(preg_match_strings('/no matches/', $s, $m2, $offset));
  var_dump($m2);

  /** @var mixed[] $m2 */
  $m3 = [];
  var_dump(preg_match_strings('/\d+/', $s, $m3, $offset));
  var_dump($m3);
}

function test_re_string_string(string $s) {
  /** @var string[] $m1 */
  $m1 = [];
  var_dump(preg_match_strings('/\d+/', $s, $m1));
  var_dump($m1);

  var_dump(preg_match_strings('/no matches/', $s, $m2));
  var_dump($m2);
}

function test_string_string_mixed(string $re, string $s) {
  /** @var mixed $m1 */
  $m1 = [];
  var_dump(preg_match_strings($re, $s, $m1));
  var_dump($m1);

  /** @var array $m2 */
  $m2 = [];
  var_dump(preg_match_strings($re, $s, $m2));
  var_dump($m2);

  /** @var mixed[] $m2 */
  $m3 = [];
  var_dump(preg_match_strings($re, $s, $m3));
  var_dump($m3);
}

function test_string_string_string(string $re, string $s) {
  /** @var string[] $m1 */
  $m1 = [];
  var_dump(preg_match_strings($re, $s, $m1));
  var_dump($m1);

  var_dump(preg_match_strings($re, $s, $m2));
  var_dump($m2);
}

/** @param mixed $re */
function test_mixed_string_mixed($re, string $s) {
  /** @var mixed $m1 */
  $m1 = [];
  var_dump(preg_match_strings($re, $s, $m1));
  var_dump($m1);

  /** @var array $m2 */
  $m2 = [];
  var_dump(preg_match_strings($re, $s, $m2));
  var_dump($m2);

  /** @var mixed[] $m2 */
  $m3 = [];
  var_dump(preg_match_strings($re, $s, $m3));
  var_dump($m3);
}

/** @param mixed $re */
function test_mixed_string_string($re, string $s) {
  /** @var string[] $m1 */
  $m1 = [];
  var_dump(preg_match_strings($re, $s, $m1));
  var_dump($m1);

  var_dump(preg_match_strings($re, $s, $m2));
  var_dump($m2);
}

/**
 * @param mixed $re
 * @param mixed $s
 * @param mixed $offset
 */
function test_mixed_mixed_mixed_offset($re, $s, $offset) {
  /** @var mixed $m1 */
  $m1 = [];
  var_dump(preg_match_strings($re, $s, $m1, $offset));
  var_dump($m1);

  /** @var array $m2 */
  $m2 = [];
  var_dump(preg_match_strings($re, $s, $m2, $offset));
  var_dump($m2);

  /** @var mixed[] $m2 */
  $m3 = [];
  var_dump(preg_match_strings($re, $s, $m3, $offset));
  var_dump($m3);
}

$regexps = ['/\d+/', '/no matches/', '/^foo/', '/.*/', '/#?/'];
$inputs = ['430 43 1', '1', 'example', 'foo', '-40', 'hello world', 'longer string with numbers (43) and # symbol'];
$offsets = [0, 1, 3, 10, 30, 100, -1, -3, -100];

foreach ($regexps as $re) {
  foreach ($inputs as $s) {
    foreach ($offsets as $offset) {
      test_re_string_mixed($s);
      test_re_string_string($s);
      test_string_string_mixed($re, $s);
      test_string_string_string($re, $s);
      test_mixed_string_mixed($re, $s);
      test_mixed_string_string($re, $s);

      test_re_string_mixed_offset($s, $offset);
      test_mixed_mixed_mixed_offset($re, $s, $offset);
    }
  }
}
