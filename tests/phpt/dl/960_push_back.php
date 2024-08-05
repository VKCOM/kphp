@ok k2_skip
<?php


$arr1 = array();
$arr2 = array("2");
$arr1[] = array("3");
$arr1 = $arr2;

function def_arg ($a = "test") {
}

def_arg ();

function test_ret() {
  return $a[10] = 123;
}
test_ret();

preg_match('/^([A-Z0-9_]+?)(: (.+))?$/', "wft", $matches);
$tmp = $matches[4];
$str = "hello";
$str = $matches[2];

function f_var($a) {
  $a[1] = "tests";
  f_string ($a[1]);
}

function f_string($a) {
}

f_string ("hello");
f_var ($_SERVER);

function g($a) {
  print $a;
}
g("hello");

$x = "test";
g($x);
$x = array();

define ('A', 123 + 432);

function f() {
  print A;
  print "\n";
  $_SERVER = "hello";
}
f();
print $_SERVER;
print "\n";

$a = $b[4] = 123;
print_r ($a."\n");
print_r ($b);
print_r ("\n");
$a = $b[] = 123;
print_r ($a."\n");
print_r ($b);
print_r ("\n");
