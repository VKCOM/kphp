@ok
<?php

var_dump(array_pad(array(), 0, 0));
var_dump(array_pad(array(), -1, 0));
var_dump(array_pad(array("", -1, 2.0), 5, 0));
var_dump(array_pad(array("", -1, 2.0), 5, array()));
var_dump(array_pad(array("", -1, 2.0), 2, array()));
var_dump(array_pad(array("", -1, 2.0), -3, array()));
var_dump(array_pad(array("", -1, 2.0), -4, array()));


// var_dump(array_pad(array("", -1, 2.0), 2000000, 0));
// var_dump(array_pad("", 2000000, 0));

$input = array (
  array(1, 2, 3),
  array("a" => 10, 77 => "b", 12 => "c"),
  array("hello", 'world'),
  array("one" => 1, "two" => 2)
);
$pad_size = 5;
$pad_value = "HELLO";

var_dump(array_pad($input, $pad_size, $pad_value));
var_dump(array_pad($input, -$pad_size, $pad_value));

foreach ($input as &$a) {
    var_dump(array_pad($a, $pad_size, $pad_value));
    var_dump(array_pad($a, -$pad_size, $pad_value));
}
unset($a);

var_dump($input);
