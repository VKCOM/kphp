@ok
<?php

function extend_test_suite($tests) {
  $all_tests = [];
  foreach ($tests as $test) {
    $all_tests[] = $test;
    $all_tests[] = "  $test";
    $all_tests[] = "$test  ";
    $all_tests[] = "  $test  ";
    $all_tests[] = "$test$test";
    $all_tests[] = "$test _ $test";
    $all_tests[] = "\x00$test";
    $all_tests[] = "$test\x00$test\x00";
  }
  return $all_tests;
}

function test_strtolower_strtoupper() {
  $tests = extend_test_suite([
    '',
    ' ',
    'a',
    'A',
    '_',
    'foo',
    'foO',
    'fOo',
    'fOO',
    'Foo',
    'FoO',
    'FOo',
    'FOO',
    'aAaA',
    'AAAA',
    'aaAa',
    'Hello, World!',
    'hello, world!',
    'HELLO, WORLD!',
    '93258324',
    '&^@$%#^$@',
  ]);
  foreach ($tests as $test) {
    var_dump(strtolower(strtolower($test)));
    var_dump(strtolower($test));
    var_dump(strtolower(strtoupper($test)));
    var_dump(strtoupper($test));
    var_dump(strtoupper(strtolower($test)));
    var_dump(strtoupper(strtoupper($test)));
  }
}

test_strtolower_strtoupper();

