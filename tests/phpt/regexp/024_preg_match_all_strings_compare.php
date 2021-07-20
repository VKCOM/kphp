@ok
<?php

function test1($re, $subject) {
  /** @var string[] */
  $s = [];
  if (preg_match_all($re, $subject, $m)) {
    $s = $m[1];
  }
  var_dump([!empty($s)]);
}

function test2($re, $subject) {
  $s = false;
  if (preg_match_all($re, $subject, $m)) {
    $s = $m[1];
  }
  var_dump([!empty($s)]);
}

function test3($re, $subject) {
  preg_match_all($re, $subject, $m);
  var_dump([!empty($m), !empty($m[0])]);
}

$inputs = [
  ['/x(\d+)/', 'hello x40'],
  ['/x(\d+)/', 'hello'],
  ['/x\d+/', 'hello x40'],
  ['/x\d+/', 'hello'],
  ['/x()y/', 'hello xy'],
  ['/x()y/', 'hello x'],
  ['/x(.*)y/', 'hello xy'],
  ['/x(.*)y/', 'hello x0y'],
  ['/x(.*)y/', 'hello x'],
  ['/a/', 'aaa'],
  ['/aa/', 'aaaaa'],
];

foreach ($inputs as $input) {
  list ($pattern, $subject) = $input;
  echo "pattern=$pattern subject=$subject\n";
  test1($pattern, $subject);
  test2($pattern, $subject);
  test3($pattern, $subject);
}
