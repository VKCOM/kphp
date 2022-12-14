@ok
<?php

function test() {
  $map_string_parts = [
    10 => '',
    20 => 'foo',
    30 => '',
    40 => 'hello, world',
    50 => ' ',
    60 => '?',
  ];

  var_dump(implode('', $map_string_parts));
  var_dump(implode('/', $map_string_parts));
  var_dump(implode('//', $map_string_parts));

  /** @var string[][] $string_parts */
  $string_parts = [
    [],
    ['foo'],
    [' ', 'foo'],
    ['foo', ' ', 'bar'],
  ];

  /** @var int[][] $int_parts */
  $int_parts = [
    [],
    [1],
    [5324, -24],
    [2394, 341, 0],
  ];

  /** @var mixed[][] $mixed_parts */
  $mixed_parts = [
    ['0'],
    ['-53'],
    ['3525', '53253'],
  ];
  foreach ($string_parts as $string_part) {
    $mixed_parts[] = $string_part;
  }
  foreach ($int_parts as $int_part) {
    $mixed_parts[] = $int_parts;
  }

  foreach ($string_parts as $string_part) {
    var_dump(implode(', ', $string_part));
    var_dump(implode(' ', $string_part));
    var_dump(implode('', $string_part));
  }
  foreach ($int_parts as $int_part) {
    var_dump(implode(', ', $int_part));
    var_dump(implode(' ', $int_part));
    var_dump(implode('', $int_part));
  }
  foreach ($mixed_parts as $mixed_part) {
    var_dump(implode(', ', $mixed_part));
    var_dump(implode(' ', $mixed_part));
    var_dump(implode('', $mixed_part));
  }

  $str_arr = ['hello'];
  $res = implode(' ', $str_arr);
  var_dump($res);
  $res[0] = 'x';
  var_dump($res);
  var_dump($str_arr[0]);

  $str_arr2 = ['hello'];
  $res2 = implode(' ', $str_arr2);
  var_dump($res2);
  $str_arr2[0][1] = 'x';
  var_dump($res2);
  var_dump($str_arr2[0]);

  $str_arr3 = ['foo' => 'hello'];
  $res3 = implode(' ', $str_arr3);
  var_dump($res3);
  $res3[2] = 'x';
  var_dump($res3);
  var_dump($str_arr3['foo']);
}

test();
