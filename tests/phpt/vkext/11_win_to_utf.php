@ok
<?php

$s1 = '';
$s2 = ' ';
$s3 = "\x74";
$s4 = "\x74\x65\x73\x74";
$s5 = "\x54\x45\x53\x54";
$s6 = &$s5;

var_dump(vk_win_to_utf8($s1));
var_dump(vk_win_to_utf8($s2));
var_dump(vk_win_to_utf8($s3));
var_dump(vk_win_to_utf8($s4));
var_dump(vk_win_to_utf8($s5));
var_dump(vk_win_to_utf8($s6));

?>

--EXPECT--
string(0) ""
string(1) " "
string(1) "t"
string(4) "test"
string(4) "TEST"
string(4) "TEST"
