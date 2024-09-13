@ok k2_skip
<?php

/*
$a_int = 1;
$b_int = 2;
$b_float = 2.0;
$c_string = "tests";

$c1 = var_if_neq ($a_int, $b_int);
var_dump ($c1);

$c2 = var_if_neq ($a_int, $b_float);
var_dump ($c2);

$c3 = var_if_neq ($c_string, $b_float);
var_dump ($c3);
*/

$a = array (1, 2, 3, 4, 5);
$b = array_chunk ($a, 2);
var_dump ($b);
$c = array_shift ($a);

$a = array (array (1, 2, 3));
list ($x, $y, $z) = $a;

global $i_keywords, $i_char_lexems, $i_2char_lexems, $i_3char_lexems;

$i_2char_lexems = array (
 1 => array (1=>"test"),
);


$i_2char_lexems[0][0][0] = 123;

/** test bool and MC **/
/*function first_test() {
  $mc = false;
  $mc = new Memcache();
}

first_test();*/

function f() {
  $x = 5;
  var_dump ($x);
}
f();

print (2 + 3) . 5;

function second_test() {
  $a = array (1, 2, 3);
  $b = array (1, 2, 3);
  $c = array (1, 2, 3);
  $d = $b;
  $b[1] = 3;
  foreach ($c as $x) {
    print $x."\n";
  }
}
second_test();

function third_test() {
  $es = array (array (1, 2, 3));
  foreach ($es as $e) {
    $e[1] = 123;
  } 

  $fs = array (array (1, 2, 3));
  foreach ($fs as &$f) {
    $f[1] = 123;
  } 

  $c = 1;
  $c = false;

  $a = array (array (1, 2, 3));
  $c = $a[0];
}

third_test();

function forth_test() {
  $a = "hello";
  echo substr ($a, 1);

  $arr_1 = array (1, 2, 3);
  $arr_2 = false;
  $arr_2 = array (0, 5, 9);
  $arr = array_merge ($arr_1, $arr_2);
  $arr = array_merge ($arr_1, $arr_2);
  var_dump ($arr);
}

forth_test();

function fifth_test () {
  $arr = array (1=>2, 3=>4, 5=>"hello");
  $id = 1;
  $v[5] = "test";
  $v[$id] = $arr;


  $g = str_replace(array('<', '>', '&'), array('&lt;', '&gt;', '&amp;'), "hello");
}

fifth_test();

$a = false;
$a = 1;
$a = array (array (1, 2, 3));
