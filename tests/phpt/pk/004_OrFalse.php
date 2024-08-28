@ok k2_skip
<?php

function test_all(){
	global $x_int, $x_string, $x_array;

	var_dump(($x_int != false) ? $x_int : 2);
	var_dump(($x_int != 1) ? $x_int : 2);
	var_dump(($x_int != true) ? $x_int : 2);

	var_dump(($x_string != false) ? $x_string : "hey");
	var_dump(($x_string != "hi") ? $x_string : "hey");
	var_dump(($x_string != 1) ? $x_string : "hey");
	var_dump(($x_string != true) ? $x_string : "hey");

	var_dump(($x_array != false) ? $x_array : array(2, 3));
	var_dump(($x_array != array(1, 2)) ? $x_array : array(2, 3));
	var_dump(($x_array != true) ? $x_array : array(2, 3));
}

$x_int = false;
$x_string = false;
$x_array = false;

test_all();

$x_int = 1;
$x_string = "hi";
$x_array = array(1, 2);

test_all();

$x_int = 3;
$x_string = "1";
$x_array = array(5, 6);

test_all();

var_dump($x_int == 5 ? $x_int : array(3, 4));
var_dump(($x_int == 5) ? $x_int : (($x_string == 6) ? 1 : 2));
var_dump(($x_int == 5) ? $x_int : (($x_string == 6) ? 1 : false));
var_dump(1 ? (2 ? 3 : false) : 5);

var_dump (@realpath ("there_is_no_such_a_file"));