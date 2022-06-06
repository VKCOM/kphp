@ok
<?php

#ifndef KPHP
function preg_match_strings($pat, $subj, &$m, $offset = 0) {
  return preg_match($pat, $subj, $m, 0, $offset);
}

function preg_match_all_strings($pat, $subj, &$m, $offset = 0) {
  return preg_match_all($pat, $subj, $m, 0, $offset);
}
#endif

function ensure_string(string $s) {}

/** @param string[] $a */
function ensure_string_array($a) {}

// test preg_match_strings by calling it directly
function test_preg_match_strings(string $pat, string $s, int $offset) {
  if (preg_match_strings($pat, $s, $matches)) {
    ensure_string($matches[0]);
    var_dump($matches[0]);
  }
  if (preg_match_strings($pat, $s, $matches2, 0)) {
    ensure_string($matches2[0]);
    var_dump($matches2[0]);
  }
  $m = [];
  if (preg_match_strings($pat, $s, $m, $offset)) {
    ensure_string($m[0]);
    var_dump($m[0]);
  }
}

// test preg_match_all_strings by calling it directly
function test_preg_match_all_strings(string $pat, string $s, int $offset) {
  if (preg_match_all_strings($pat, $s, $matches)) {
    ensure_string_array($matches[0]);
    var_dump($matches[0]);
  }
  if (preg_match_all_strings($pat, $s, $matches2, 0)) {
    ensure_string_array($matches2[0]);
    var_dump($matches2[0]);
  }
  $m = [];
  if (preg_match_all_strings($pat, $s, $m, $offset)) {
    ensure_string_array($m[0]);
    var_dump($m[0]);
  }
}

// test preg_match_strings via func specialization
function test_preg_match(string $pat, string $s, int $offset) {
  if (preg_match($pat, $s, $matches)) {
    ensure_string($matches[0]);
    var_dump($matches[0]);
  }
  if (preg_match($pat, $s, $matches2, 0)) {
    ensure_string($matches2[0]);
    var_dump($matches2[0]);
  }
  $m = [];
  if (preg_match($pat, $s, $m, 0, $offset)) {
    ensure_string($m[0]);
    var_dump($m[0]);
  }
}

// test preg_match_all_strings via func specialization
function test_preg_match_all(string $pat, string $s, int $offset) {
  if (preg_match_all($pat, $s, $matches)) {
    ensure_string_array($matches[0]);
    var_dump($matches[0]);
  }
  if (preg_match_all($pat, $s, $matches2, 0)) {
    ensure_string_array($matches2[0]);
    var_dump($matches2[0]);
  }
  $m = [];
  if (preg_match_all($pat, $s, $m, 0)) {
    ensure_string_array($m[0]);
    var_dump($m[0]);
  }
}

$input_strings = [
  '',
  '499',
  '+32',
  '-924',
  '23 439',
  '1 112 43',
  'word',
  'hello, world',
];
$all_input_strings = [];
foreach ($input_strings as $s) {
  $all_input_strings[] = $s;
  $all_input_strings[] = $s . $s . $s;
  $all_input_strings[] = ' ' . $s;
  $all_input_strings[] = $s . ' ';
  $all_input_strings[] = ' ' . $s . ' ';
  $all_input_strings[] = "\x00" . $s;
  $all_input_strings[] = $s . "\x00";
}
$offsets = [0, -1, 1, -3, 3, 10, 100];

$patterns = [
  '/^$/',
  '/\d+/',
  '/^\d+$/',
  '/.*/',
  '/hello, world/',
  '/\d.*\d/',
];

foreach ($patterns as $pat) {
  foreach ($all_input_strings as $s) {
    foreach ($offsets as $offset) {
      test_preg_match_strings($pat, $s, $offset);
      test_preg_match_all_strings($pat, $s, $offset);
      test_preg_match($pat, $s, $offset);
      test_preg_match_all($pat, $s, $offset);
    }
  }
}
