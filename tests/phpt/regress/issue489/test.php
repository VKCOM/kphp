@ok
<?php

function test_string_index() {
  $s = 'hello';
  $indexes = [' 1', '+1', ' +1', '  +1', '    1', 1];
  foreach ($indexes as $index) {
    var_dump($s[$index]);
  }
}

function test_intval($x) {
  var_dump("`$x`: intval = " . intval($x));
  var_dump("`$x`: intcast = " . (int)($x));
}

function test_array_key($x) {
  $arr = [$x => 10];
  $arr2 = [intval($x) => 10];
  var_dump($arr);
  var_dump($arr2);
}

test_string_index();

$values = [
  '1', '12439', '14a', '0x5', 'ab',
  '0a', '00b', '00', '001',
];
$all_values = [];
foreach ($values as $x) {
  $all_values[] = $x;
  $all_values[] = "\n" . $x;
  $all_values[] = $x . "\n";
  $all_values[] = ' ' . $x;
  $all_values[] = $x . ' ';
  $all_values[] = ' ' . $x . ' ';
  $all_values[] = '  +' . $x . ' ';
  $all_values[] = '  -' . $x . ' ';
  $all_values[] = '+ ' . $x;
  $all_values[] = '++ ' . $x;
  $all_values[] = '+-' . $x;
}
foreach ($all_values as $x) {
  test_intval($x);
  test_array_key($x);
}
