@ok
<?php

// Tests for the preg_match and preg_match_all function overloading with 2 args (without $matches).

function test_preg_match(string $re, $s, $offset) {
  var_dump(["preg_match('$re', $s, _, 0, $offset)" => preg_match($re, $s, $m, 0, $offset)]);

  /** @var mixed $mixed_re */
  $mixed_re = $re;
  var_dump(["preg_match((mixed)'$re', $s, _, 0, $offset)" => preg_match($mixed_re, $s, $m, 0, $offset)]);
}

function test_preg_match_all(string $re, $s, $offset) {
  var_dump(["preg_match_all('$re', $s, _, 0, $offset)" => preg_match_all($re, $s, $m, 0, $offset)]);

  /** @var mixed $mixed_re */
  $mixed_re = $re;
  var_dump(["preg_match_all((mixed)'$re', $s, _, 0, $offset)" => preg_match_all($mixed_re, $s, $m, 0, $offset)]);
}

$patterns = [
  '/\d/',
  '/\d+/',
  '/^\d+$/',
  '/.*/',
  '/\s/',
  '/\s+/',
  '/\s*/',
  '/\w+/',
  '/\w*/',
  '/foo/',
  '/^foo$/',
  '/\bfoo\b/',
];

$inputs = [
  '',
  ' ',
  '   ',
  '1',
  '123',
  '1 2',
  '12 34',
  '   123921 821488123 4123',
  '   123921 foo 4123',
  'foo',
  'foofoo',
  'foo foo',
  '<foo>foofoo',
];

$offsets = [0, 1, -1, 3, 10, -10, 100, -100];

foreach ($offsets as $offset) {
  foreach ($patterns as $pattern) {
    foreach ($inputs as $s) {
      test_preg_match($pattern, $s, $offset);
      test_preg_match_all($pattern, $s, $offset);
    }
  }
}
