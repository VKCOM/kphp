@ok
<?php
$a = "123";
$b = "123";

var_dump(vk_json_encode($b));
var_dump(vk_json_encode(""));
var_dump(vk_json_encode(" "));
var_dump(vk_json_encode("a"));
var_dump(vk_json_encode(1));
var_dump(vk_json_encode(true));
var_dump(vk_json_encode(1.1));
var_dump(vk_json_encode(-1));

var_dump(vk_json_encode(array(1, 2, 3)));
var_dump(vk_json_encode(array(1., 2, 3)));
var_dump(vk_json_encode(array(true, 2, 3)));
var_dump(vk_json_encode(array(true, .1, 3)));


var_dump(vk_json_encode(array("1" => 1, "2" => true, 4 => "4", 5 => 5.)));
var_dump(vk_json_encode(array("1" => 1, "b" => true, 4 => "c", 5 => 5.)));
var_dump(vk_json_encode(array("1" => array(1, 2), "b" => true, 4 => "c", 5 => 5.)));

$a = 1;
$b = 1;
$c = array(1 => $b);
var_dump(vk_json_encode($c));

$a = array(1 => 10, 3 => 20);
$b = array(4 => 10, 6 => 20, 7 => $a);
$a[4] = $b;
var_dump(vk_json_encode($a));

$r = [
  'test' => 'test',
  'array' => []
];

$r2 = [
  $r,
  'test'
];

$r['array'][] = $r;
$r['r2'] = $r2;
$r['test3'] = $r;

$k = vk_json_encode($r);
var_dump($k);

// fix for php7
var_dump(vk_json_encode(['foo' => 1, '' => 2, 0 => 3, false => 4, null => 5]));
var_dump(vk_json_encode(['foo' => 1, 0 => 3, false => 4, null => 5]));
var_dump(vk_json_encode(['foo' => 1, false => 4, null => 5]));
var_dump(vk_json_encode(['foo' => 1, null => 5]));
var_dump(vk_json_encode([null => 5]));
var_dump(vk_json_encode([5]));
var_dump(vk_json_encode(['' => 2, false => 4, null => 5]));
var_dump(vk_json_encode(['' => 2]));

?>

--EXPECT--
string(5) ""123""
string(2) """"
string(3) "" ""
string(3) ""a""
string(1) "1"
string(4) "true"
string(8) "1.100000"
string(2) "-1"
string(7) "[1,2,3]"
string(14) "[1.000000,2,3]"
string(10) "[true,2,3]"
string(17) "[true,0.100000,3]"
string(37) "{"1":1,"2":true,"4":"4","5":5.000000}"
string(37) "{"1":1,"b":true,"4":"c","5":5.000000}"
string(41) "{"1":[1,2],"b":true,"4":"c","5":5.000000}"
string(7) "{"1":1}"
string(37) "{"4":null,"6":20,"7":{"1":10,"3":20}}"
string(62) "{"test":"test","array":[null],"r2":[null,"test"],"test3":null}"
string(20) "{"foo":1,"":5,"0":4}"
string(20) "{"foo":1,"0":4,"":5}"
string(20) "{"foo":1,"0":4,"":5}"
string(14) "{"foo":1,"":5}"
string(6) "{"":5}"
string(3) "[5]"
string(12) "{"":5,"0":4}"
string(6) "{"":2}"
