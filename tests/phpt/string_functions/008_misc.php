@ok k2_skip
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

function test_ord() {
  var_dump(ord("a"));
  var_dump(ord("z"));
  var_dump(ord("0"));
  var_dump(ord("9"));
  var_dump(ord("!"));
  var_dump(ord("*"));
  var_dump(ord("@"));
  var_dump(ord("\n"));
  var_dump(ord("\x0A"));
  var_dump(ord("\xFF"));
  var_dump(ord("Hello"));
}

function test_string_conv() {
  /** @var mixed $mixed */
  $mixed = 43;
  /** @var ?int $optional_int1 */
  $optional_int1 = null;
  /** @var ?int $optional_int2 */
  $optional_int2 = 3249;
  $int = -4;
  $float = 42.6;
  $s = 'foo';
  /** @var ?string $optional_s1 */
  $optional_s1 = null;
  /** @var ?string $optional_s2 */
  $optional_s2 = 'bar';

  var_dump('' . substr($mixed, 1));
  var_dump(substr($mixed, 1) . '');

  var_dump('' . $mixed);
  var_dump('' . $optional_int1);
  var_dump('' . $optional_int2);
  var_dump('' . $int);
  var_dump('' . $float);
  var_dump('' . $optional_s1);
  var_dump('' . $optional_s2);

  var_dump($mixed . '');
  var_dump($optional_int1 . '');
  var_dump($optional_int2 . '');
  var_dump($int . '');
  var_dump($float . '');
  var_dump($optional_s1 . '');
  var_dump($optional_s2 . '');

  var_dump("$mixed");
  var_dump("$optional_int1");
  var_dump("$optional_int2");
  var_dump("$int");
  var_dump("$float");
  var_dump("$optional_s1");
  var_dump("$optional_s2");
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
test_ord();
test_string_conv();
