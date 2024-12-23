@ok
<?php

$s1 = '';
$s2 = 'a';
$s3 = 'A';
$s4 = 'aBc';
$s5 = 'Abc';
$s6 = 'abC';
$s7 = 'AbCdeFG';
$s8 = ' ';
$s9 = $s7;

var_dump(vk_sp_to_lower($s1));
var_dump(vk_sp_to_lower($s2));
var_dump(vk_sp_to_lower($s3));
var_dump(vk_sp_to_lower($s4));
var_dump(vk_sp_to_lower($s5));
var_dump(vk_sp_to_lower($s6));
var_dump(vk_sp_to_lower($s7));
var_dump(vk_sp_to_lower($s8));
var_dump(vk_sp_to_lower($s9));

?>

--EXPECT--
string(0) ""
string(1) "a"
string(1) "a"
string(3) "abc"
string(3) "abc"
string(3) "abc"
string(7) "abcdefg"
string(1) " "
string(7) "abcdefg"
