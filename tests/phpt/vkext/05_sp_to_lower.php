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

var_dump(vk_sp_to_lower($s1) === "");
var_dump(vk_sp_to_lower($s2) === "a");
var_dump(vk_sp_to_lower($s3) === "a");
var_dump(vk_sp_to_lower($s4) === "abc");
var_dump(vk_sp_to_lower($s5) === "abc");
var_dump(vk_sp_to_lower($s6) === "abc");
var_dump(vk_sp_to_lower($s7) === "abcdefg");
var_dump(vk_sp_to_lower($s8) === " ");
var_dump(vk_sp_to_lower($s9) === "abcdefg");

?>
