@ok k2_skip
<?php

function test_array_int64_indexing() {
  /** @var $a string[] */
  $a = ["hello", "ggg"];
  $a[4294967296] = "world";
  var_dump($a[4294967296]);
  var_dump($a);

  $a = ["hello", "ggg"] ;
  $a["4294967296"] = "world";
  var_dump($a["4294967296"]);
  var_dump($a);

  $a = ["hello", "ggg"];
  $a[4294967295] = "max uint32";
  $a[4294967296] = "max uint32 + 1";
  $a[2147483647] = "max int32";
  $a[-2147483648] = "min int32";
  $a[9223372036854775807] = "max int64";
  $a[-9223372036854775808] = "min int64";
  var_dump($a);

  var_dump(isset($a[4294967295]));
  var_dump($a[4294967295]);
  var_dump($a[4294967295] ?? "111");
  unset($a[4294967295]);
  var_dump($a[4294967295] ?? "222");

  var_dump(isset($a[2147483647]));
  var_dump($a[2147483647]);
  var_dump($a[2147483647] ?? "333");
  unset($a[2147483647]);
  var_dump($a[2147483647] ?? "444");

  var_dump(isset($a[-2147483648]));
  var_dump($a[-2147483648]);
  var_dump($a[-2147483648] ?? "555");
  unset($a[-2147483648]);
  var_dump($a[-2147483648] ?? "666");

  var_dump(isset($a[9223372036854775807]));
  var_dump($a[9223372036854775807]);
  var_dump($a[9223372036854775807] ?? "777");
  unset($a[9223372036854775807]);
  var_dump($a[9223372036854775807] ?? "888");

  var_dump(isset($a[-9223372036854775808]));
  var_dump($a[-9223372036854775808]);
  var_dump($a[-9223372036854775808] ?? "999");
  unset($a[-9223372036854775808]);
  var_dump($a[-9223372036854775808] ?? "000");
}

function test_array_var_int64_indexing() {
  /** @var $a mixed */
  $a = 1 ? ["hello", "ggg"] : "world";
  $a[4294967296] = "world";
  var_dump($a[4294967296]);
  var_dump($a);

  $a = 1 ? ["hello", "ggg"] : "world";
  $a["4294967296"] = "world";
  var_dump($a["4294967296"]);
  var_dump($a);

  $a = 1 ? ["hello", "ggg"] : "world";
  $a[4294967295] = "max uint32";
  $a[4294967296] = "max uint32 + 1";
  $a[2147483647] = "max int32";
  $a[-2147483648] = "min int32";
  $a[9223372036854775807] = "max int64";
  $a[-9223372036854775808] = "min int64";
  var_dump($a);

  var_dump(isset($a[4294967295]));
  var_dump($a[4294967295]);
  var_dump($a[4294967295] ?? "111");
  unset($a[4294967295]);
  var_dump($a[4294967295] ?? "222");

  var_dump(isset($a[2147483647]));
  var_dump($a[2147483647]);
  var_dump($a[2147483647] ?? "333");
  unset($a[2147483647]);
  var_dump($a[2147483647] ?? "444");

  var_dump(isset($a[-2147483648]));
  var_dump($a[-2147483648]);
  var_dump($a[-2147483648] ?? "555");
  unset($a[-2147483648]);
  var_dump($a[-2147483648] ?? "666");

  var_dump(isset($a[9223372036854775807]));
  var_dump($a[9223372036854775807]);
  var_dump($a[9223372036854775807] ?? "777");
  unset($a[9223372036854775807]);
  var_dump($a[9223372036854775807] ?? "888");

  var_dump(isset($a[-9223372036854775808]));
  var_dump($a[-9223372036854775808]);
  var_dump($a[-9223372036854775808] ?? "999");
  unset($a[-9223372036854775808]);
  var_dump($a[-9223372036854775808] ?? "000");

  var_dump($a);
}

function test_string_int64_indexing() {
  /** @var $s string */
  $s = "hello";

  var_dump($s[4294967295]);
  var_dump($s[4294967295] ?? "111");

  var_dump($s[2147483647]);
  var_dump($s[2147483647] ?? "333");

  var_dump($s[-2147483648]);
  var_dump($s[-2147483648] ?? "555");

  var_dump($s[9223372036854775807]);
  var_dump($s[9223372036854775807] ?? "777");

  var_dump($s[-9223372036854775808]);
  var_dump($s[-9223372036854775808] ?? "999");
}

function test_string_mixed_int64_indexing() {
  /** @var $s mixed */
  $s = 1 ? "hello"  : ["world"];

  var_dump(isset($s[4294967295]));
  var_dump($s[4294967295]);
  var_dump($s[4294967295] ?? "111");

  var_dump(isset($s[2147483647]));
  var_dump($s[2147483647]);
  var_dump($s[2147483647] ?? "333");

  var_dump(isset($s[-2147483648]));
  var_dump($s[-2147483648]);
  var_dump($s[-2147483648] ?? "555");

  var_dump(isset($s[9223372036854775807]));
  var_dump($s[9223372036854775807]);
  var_dump($s[9223372036854775807] ?? "777");

  var_dump(isset($s[-9223372036854775808]));
  var_dump($s[-9223372036854775808]);
  var_dump($s[-9223372036854775808] ?? "999");
}

test_array_int64_indexing();
test_array_var_int64_indexing();
test_string_int64_indexing();
test_string_mixed_int64_indexing();
