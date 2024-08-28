@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

define('one', 1);
$arr = [0 => 1, one => 2];
$raw_arr = base64_encode(msgpack_serialize($arr));
var_dump($raw_arr);

$arr = [0 => 'a', 1 => "12", 2 => "asdzxcv"];
$arr2 = ['a'];
$raw_arr = base64_encode(msgpack_serialize($arr));
var_dump($raw_arr);


