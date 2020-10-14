@ok
<?php

$x_var = "as";
echo is_string ($x_var);

$y_var = "as";
echo isset ($y_var);


$arr3[1][2][3] = 123;
$arr1 = $arr3[1];
$arr1[1] = 12;
//$arr1;
//$arr2;
//$tmp = $arr1[1];
//$arr1 = $arr2[1];

$c = array (1, 2, 3, "hello", array (1=>23));
print_r ($c);

$b = array();
$b = array ("hello", $b);
$b = array(array(array(array(array($b)))));

print_r ($b);



$a = array(array(array(array(array("hello", "world")))));
/**
 * @param int $n
 * @return mixed[][][][][][]
 */
function f($n) {
  if ($n == 0) {
    return array();
  }
  return array (f($n - 1));
} 

$x = f(9);

var_dump ($x);

$a = array ($a);
print_r ($a);

