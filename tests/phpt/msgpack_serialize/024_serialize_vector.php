@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function arrayKeysToWin(array $val): array {
  foreach($val as $k => $v) {
    unset($val[$k]);
    $val[vk_utf8_to_win($k)] = $v;
  }
  return $val;
}

function test_vector() {
  $a1 = [1, 2, 3, 4, 5];
  $serialized1 = msgpack_serialize_safe($a1);
  var_dump(base64_encode($serialized1));

  $a2 = [0 => 1, 1 => 2, 2 => 3, 3 => 4, 4 => 5];
  $serialized2 = msgpack_serialize_safe($a2);
  assert_true($serialized1 === $serialized2);
  var_dump(base64_encode($serialized2));

  $a3 = ["0" => 1, "1" => 2, "2" => 3, "3" => 4, "4" => 5];
  $serialized3 = msgpack_serialize_safe($a3);
  assert_true($serialized1 === $serialized3);
  var_dump(base64_encode($serialized3));

  $a4 = ["0" => 1, 1 => 2, "2" => 3, 3 => 4, "4" => 5];
  $serialized4 = msgpack_serialize_safe($a4);
  assert_true($serialized1 === $serialized4);
  var_dump(base64_encode($serialized4));

  $a5 = [];
  $a5[] = 1;
  $a5[1] = 2;
  $a5["2"] = 3;
  $a5[] = 4;
  $a5[4] = 5;
  $serialized5 = msgpack_serialize_safe($a5);
  assert_true($serialized1 === $serialized5);
  var_dump(base64_encode($serialized5));

  $a6 = [1 => 2];
  $a6[0] = 1;
  $a6[2] = 3;
  $a6[3] = 4;
  $a6[4] = 5;
  $serialized6 = msgpack_serialize_safe($a6);
  assert_false($serialized1 === $serialized6);
  var_dump(base64_encode($serialized6));

  $a7 = arrayKeysToWin($a1);
  $serialized7 = msgpack_serialize_safe($a7);
  assert_true($serialized1 === $serialized7);
  var_dump(base64_encode($serialized7));
}

test_vector();
