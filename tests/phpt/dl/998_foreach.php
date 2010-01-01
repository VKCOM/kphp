@ok
<?php

$vect = array(1, 2, 3, 4, 5);

$a = $vect;
print_r ($a);
foreach ($a as $a_key=>&$a_val) {
  print_r ($a_val);
  print_r ($a_key);
  $a_key = $a_key + 1;
  $a_val = $a_val + 1;
}
print_r ($a);

$a = $vect;
print_r ($a);
foreach ($a as $a_key=>$a_val) {
  print_r ($a_val);
  print_r ($a_key);
  $a_key = $a_key + 1;
  $a_val = $a_val + 1;
}
print_r ($a);


$a = $vect;
print_r ($a);
foreach ($a as &$a_val) {
  print_r ($a_val);
  $a_val = $a_val + 1;
}
print_r ($a);

$a = $vect;
print_r ($a);
foreach ($a as $a_val) {
  print_r ($a_val);
  $a_val = $a_val + 1;
  $a = array(123);
}
print_r ($a);

$a = $vect;
print_r ($a);
foreach ($a as $a_val) {
  print_r ($a_val);
  $a_val = $a_val + 1;
}
print_r ($a);

#Compilation error
//function f() {
//  global $a;
//}
//function g() {
//  global $a;
//  foreach ($a as &$a_val) {
//    # $a = array();
//    //  f(); 
//    $a = 123;
//  }
//}
//g();
//foreach ($a as &$a_val) {
//  $a = array();
//  f(); 
//}


?>
