@ok
<?php
function f($x) {
  static $y = 0;
  print ("x = {$x}, y = {$y}\n");
  $y++;
  return $y;
}

function g($x) {
  static $y = 1;
  print ("x = {$x}, y = {$y}\n");
  $y++;
  return $y;
}

function h($x) {
  static $y = 2;
  static $smth = array ("hello", 123);
  print ("x = {$x}, y = {$y}\n");
  var_dump ($smth);
  $y++;
  return $y;
}

for ($x = 0; $x < 5; $x++) {
  f ($x);
  g ($x);
  h ($x);
} 

