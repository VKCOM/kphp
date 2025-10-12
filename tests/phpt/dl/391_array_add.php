@ok
<?php

function f(){
  $x = array(1);
  $y = $x;
  $x += array(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
  var_dump($x);
  var_dump($y);
  $z = array(1);
  $z[0] = 2;
  var_dump($z);
  var_dump($y);
}

function g(){
  $x = array(1);
  $x += array(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
  var_dump($x);

  $z = array(1);
  $z[0] = 2;
  var_dump($z);
}


f();
g();
