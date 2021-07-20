@ok
<?php

function test1($re, $subject) {
  $s = '';
  if (preg_match($re, $subject, $m)) {
    $s = $m[1];
  }
  var_dump([!empty($s)]);
}

function test2($re, $subject) {
  $s = false;
  if (preg_match($re, $subject, $m)) {
    $s = $m[1];
  }
  var_dump([!empty($s)]);
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
];

foreach ($inputs as $input) {
  list ($pattern, $subject) = $input;
  echo "pattern=$pattern subject=$subject\n";
  test1($pattern, $subject);
  test2($pattern, $subject);
}
