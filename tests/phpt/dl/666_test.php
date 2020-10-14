@ok K.O.T.
<?php
function gcd ($a, $b = 60) {
  return $b ? gcd ($b, $a % $b) : $a;
}

var_dump (gcd (34, 289));


function square ($x = true) {
  return $x * $x;
}

var_dump (square ());
var_dump (square (-239));
var_dump (square (-1.41));
#var_dump (square (983475345));
  

$array = array (0 => 'cat', 1 => 'kitten', "-2" => 'fish', 'fish' => 'salmon');
$array[] = 0;

var_dump ($array);
$replaced = 0;

foreach ($array as $k=>&$v) {
  if ($v == 'salmon') {
    print_r ($k);
//    $array[$k] = 'shark';
    $v = 'shark';
    $replaced++;
  }
}
unset($v);

var_dump ($array);
var_dump ($replaced);

?>
