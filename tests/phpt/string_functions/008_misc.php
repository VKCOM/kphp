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
    $all_tests[] = "+$test";
    $all_tests[] = "-$test";
    if (strlen($test) > 2) {
      $all_tests[] = substr($test, strlen($test) / 2);
    }
  }
  return $all_tests;
}

function test_strcasecmp() {
  $tests = extend_test_suite([
    '',
    ' ',
    '_',
    '134',
    'foo',
    'Foo',
    'FOO',
    'Hello, World!',
    'Hello, World',
  ]);

  foreach ($tests as $s1) {
    foreach ($tests as $s2) {
      var_dump("'$s1' -vs- '$s2' " . strcasecmp($s1, $s2));
    }
  }
}

function test_strtolower_strtoupper() {
  $tests = extend_test_suite([
    '',
    ' ',
    'a',
    'A',
    '_',
    '1242',
    '+534',
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
test_strcasecmp();
