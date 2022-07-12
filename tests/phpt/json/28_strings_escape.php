@ok
<?php
require_once 'kphp_tester_include.php';

define('TEST_STRINGS', [
 'this"with"quotes',
 "this\\with\backslashes",
 'this\\with\backslashes_single_quoted',
 "this//with/slashes",
 'this//with/slashes_single_quoted',
 "with\nnl",
 "with\bb",
 "with\fb",
 "with\rr",
 "with\rt",
 "русские буквы",
 'русские буквы',
 'key"with\\some\nescapes' => "value"
]);

class A {
  public array $arr;
}

function test_escape_strings() {
  $a = new A;
  $a->arr = TEST_STRINGS;
  $json_a = JsonEncoder::encode($a);
  $json_native = json_encode(['arr' => TEST_STRINGS], JSON_UNESCAPED_UNICODE);
  var_dump($json_a);
  var_dump($json_native);
  var_dump($json_a === $json_native);

  $a_restored = JsonEncoder::decode($json_a, A::class);
  $arr_native = json_decode($json_native, true);
  var_dump($a_restored->arr);
  var_dump($arr_native['arr']);
  var_dump($a_restored->arr === $arr_native['arr']);
}

test_escape_strings();
