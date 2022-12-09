@ok
<?php

function test_non_shared() {
  $a = array_fill(0, 10, 'str');
  array_splice($a, 8);
  var_dump($a);
  array_splice($a, 6, count($a));
  var_dump($a);
  $a_copy = $a;
  array_splice($a, 4, count($a));
  var_dump($a);
  var_dump($a_copy);
  $removed = array_splice($a, 3);
  var_dump($a);
  var_dump($removed);
  var_dump($a_copy);
  $removed2 = array_splice($a, 0, count($a));
  var_dump($a);
  var_dump($removed2);
  var_dump($a_copy);
  for ($i = 0; $i < 5; $i++) {
    $a[] = (string)($i * 2);
  }
  var_dump($a);
  var_dump($removed2);
  var_dump($a_copy);
  $removed3 = array_splice($a, 1, 2, ['a', 'b']);
  var_dump($a);
  var_dump($removed3);
  var_dump($a_copy);
  $a_copy2 = $a;
  array_splice($a, 0);
  var_dump($a);
  var_dump($removed3);
  var_dump($a_copy);
  var_dump($a_copy2);
  $a_copy2[] = 'example';
  var_dump($a);
  var_dump($removed3);
  var_dump($a_copy);
  var_dump($a_copy2);
}

function test() {
  $arrays = [
    [],
    [[]],
    [1],
    ['a', 'b', 'c'],
    [[1], [2], [3], 4, 5, 6],
    array_fill(0, 100, 0xff),
  ];

  $offsets = [
    0,
    1,
    6,
    -1,
    -2,
  ];

  $lengths = [
    0,
    1,
    10,
    -1,
  ];

  $replacements = [
    [],
    [1],
    ['r1', 'r2', 'r3'],
  ];

  foreach ($arrays as $a) {
    foreach ($offsets as $offset) {
      foreach ($lengths as $length) {
        foreach ($replacements as $replacement) {
          $a_copy = $a;
          array_splice($a_copy, $offset, $length, $replacement);
          var_dump($a_copy);
          $a_copy2 = $a;
          array_splice($a_copy2, $offset, $length);
          var_dump($a_copy2);
          $a_copy3 = $a;
          array_splice($a_copy3, $offset);
          var_dump($a_copy3);
          var_dump($a);

          $a_copy = $a;
          $removed = array_splice($a_copy, $offset, $length, $replacement);
          var_dump($removed);
          var_dump($a_copy);
          $a_copy2 = $a;
          $removed2 = array_splice($a_copy2, $offset, $length);
          var_dump($removed2);
          var_dump($a_copy2);
          $a_copy3 = $a;
          $removed3 = array_splice($a_copy3, $offset);
          var_dump($removed3);
          var_dump($a_copy3);
          var_dump($a);
        }
      }
    }
  }
}

test_non_shared();
test();
