@ok
<?php
$vect = array(1, 2, 3, 4, 5);
$a = $vect;
print_r ($a);
foreach ($a as &$a_val) {
  var_dump ($a_val);
  $a_val = $a_val + 1;
}
print_r ($a);

$a = $vect;
print_r ($a);
?>
