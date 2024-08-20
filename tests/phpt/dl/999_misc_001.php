@ok k2_skip
<?php
function sq($x) {
  return $x * $x;
}
var_dump (sq (123) );

$a = array(1, 2, 3, "4", 5);
$b = array(2, 4, "5", 6);
var_dump ($a);
var_dump ($b);
$c = array_diff ($a, $b);
var_dump ($c);
var_dump (array_diff ($a, $b));

var_dump(array_diff_key($a, $b));
?>