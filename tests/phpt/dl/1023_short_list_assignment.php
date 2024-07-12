@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function demo1() {
  $test = '1:2:3';
  $gift = array();
  [$gift['from_id'], $gift['gift_number'], $gift['id']] = explode(':', $test);

  var_dump (explode(':', $test));
  var_dump ($gift);
}

function demo2() {
  [$a, $b, $c] = tuple(1, 'a', [1,2,]);
  var_dump($a);
  var_dump($b);
  var_dump($c);
}

function demo_empty_array_elements() {
  [, $b, $c] = tuple(1, 'a', [1,2,]);
  var_dump($b);
  var_dump($c);

  [$a, , $c] = tuple(1, 'a', [1,2,]);
  var_dump($a);
  var_dump($c);

  [$a, $b, ] = tuple(1, 'a', [1,2,]);
  var_dump($a);
  var_dump($b);

  [, , $c] = tuple(1, 'a', [1,2,]);
  var_dump($c);

  [, $b, ] = tuple(1, 'a', [1,2,]);
  var_dump($b);

  [$a, , ] = tuple(1, 'a', [1,2,]);
  var_dump($a);

  [$a, $b, $c, ] = tuple(1, 'a', [1,2,]);
  var_dump($a);
  var_dump($b);
  var_dump($c);
}

function demo_in_if() {
  if([$a, $b] = [111, 222]) {
    var_dump($a);
    var_dump($b);
  }
}

function demo_in_while() {
  $pairs = [
    [1, 's'],
    [2, 'd'],
  ];
  $i = 0;
  while(true && ([$num, $s] = $pairs[$i++])) {
    var_dump($num);
    var_dump($s);
  }
  $i = 0;
  while(true && ([, $s] = $pairs[$i++])) {
    var_dump($s);
  }
}

demo1();
demo2();
demo_empty_array_elements();
demo_in_if();
demo_in_while();
