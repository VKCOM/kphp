@ok
<?php

// Tests for the preg_match and preg_match_all function overloading with 2 args (without $matches).

function test_preg_match($re, $s) {
  var_dump(["preg_match('$re', $s)" => preg_match($re, $s)]);
}

function test_preg_match_all($re, $s) {
  var_dump(["preg_match_all('$re', $s)" => preg_match_all($re, $s)]);
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
  'foo',
  'foofoo',
  'foo foo',
  '<foo>foofoo',
];

foreach ($patterns as $pattern) {
  foreach ($inputs as $s) {
    test_preg_match($pattern, $s);
    test_preg_match_all($pattern, $s);
  }
}
