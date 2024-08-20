@ok k2_skip
<?php

$ints = [
  0, 1, 7,
  32,
  100,
  1234,
  48662,
  987421,
  2312421,
  12349480,
  908374261,
  2071623429,
  2147483647, // int max
  -1, -6,
  -21,
  -231,
  -5421,
  -76542,
  -421343,
  -4213123,
  -80912429,
  -942183212,
  -2023124282,
  -2147483648 // int min
];

function test_var_dump_int() {
  global $ints;
  var_dump($ints);
  foreach($ints as $x) {
    var_dump($x);
  }
}

function test_serialize_int() {
  global $ints;
  var_dump(serialize($ints));
  foreach($ints as $x) {
    var_dump(serialize($x));
  }
}

function test_int_to_string_cast() {
  global $ints;
  foreach($ints as $x) {
    $s = (string) $x;
    var_dump($s);
  }
}
function test_int_append_to_string() {
  global $ints;
  $result = "";
  foreach($ints as $x) {
    $result .= $x;
    $result .= "\n";
  }
  var_dump($result);
}

function test_int_build_string() {
  global $ints;
  foreach($ints as $x) {
    $s = "->>  $x\n";
    var_dump($s);
  }
}

function test_brute_force() {
  for ($i = -50000; $i < 50000; ++$i) {
    echo $i;
    var_dump($i);
    $s = (string) $i;
    var_dump($s);
    $s = "->>  $i\n";
    var_dump($s);
    $s .= $i;
    var_dump($s);
  }
}

test_var_dump_int();
test_serialize_int();
test_int_to_string_cast();
test_int_append_to_string();
test_int_build_string();
test_brute_force();

