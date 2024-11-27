@ok
<?php

$s1 = '';
$s2 = ' ';
$s3 = "\xd1\x82";
$s4 = "\xd1\x82\xd0\xb5\xd1\x81\xd1\x82";
$s5 = "\xd0\xa2\xd0\x95\xd0\xa1\xd0\xa2";
$s6 = &$s5;


var_dump(vk_utf8_to_win($s1));
var_dump(vk_utf8_to_win($s2));
var_dump(vk_utf8_to_win($s3));
var_dump(vk_utf8_to_win($s4));
var_dump(vk_utf8_to_win($s5));
var_dump(vk_utf8_to_win($s6));

?>

--EXPECT--
string(0) ""
string(1) " "
string(1) "๒"
string(4) "๒ๅ๑๒"
string(4) "าลัา"
string(4) "าลัา"
