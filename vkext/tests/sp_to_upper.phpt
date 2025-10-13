--TEST--
test 'vk_sp_to_upper' function
--FILE--
<?php

$s1 = '';
$s2 = 'a';
$s3 = 'A';
$s4 = 'aBc';
$s5 = 'Abc';
$s6 = 'abC';
$s7 = 'AbCdeFG';
$s8 = ' ';
$s9 = &$s7;

var_dump(vk_sp_to_upper($s1));
var_dump(vk_sp_to_upper($s2));
var_dump(vk_sp_to_upper($s3));
var_dump(vk_sp_to_upper($s4));
var_dump(vk_sp_to_upper($s5));
var_dump(vk_sp_to_upper($s6));
var_dump(vk_sp_to_upper($s7));
var_dump(vk_sp_to_upper($s8));
var_dump(vk_sp_to_upper($s9));

?>

--EXPECT--
string(0) ""
string(1) "A"
string(1) "A"
string(3) "ABC"
string(3) "ABC"
string(3) "ABC"
string(7) "ABCDEFG"
string(1) " "
string(7) "ABCDEFG"
