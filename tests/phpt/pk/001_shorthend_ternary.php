@ok
<?php
$x = 0;
/**
 * @return int
 */
function f(){
  global $x;
  print "CALL\n";
  return 0 ?: ++$x;
}

$x = 0 ?: 1 ?: 0;
var_dump($x);
$x = f() ?: -1;
var_dump($x);
$x = -1;
$x = f() ?: -10;
var_dump($x);
$x = f() ? f() : -10;
var_dump($x);
$x = f() ?   : f() ?  : f();
var_dump($x);
$x = -1;
$x = f() ?: f() ?: f();
var_dump($x);


