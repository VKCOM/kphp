@ok
<?php
var_dump(vk_whitespace_pack('test'));

var_dump(vk_whitespace_pack(' test'));
var_dump(vk_whitespace_pack('  test'));
var_dump(vk_whitespace_pack(" \tntest"));
var_dump(vk_whitespace_pack("\t ntest"));
var_dump(vk_whitespace_pack("\n test"));
var_dump(vk_whitespace_pack(" \n test"));

var_dump(vk_whitespace_pack('test '));
var_dump(vk_whitespace_pack('test  '));
var_dump(vk_whitespace_pack("test \n"));
var_dump(vk_whitespace_pack("test \t"));
var_dump(vk_whitespace_pack("test\n\t"));
var_dump(vk_whitespace_pack("test\t  "));

var_dump(vk_whitespace_pack('test test'));
var_dump(vk_whitespace_pack('test  test'));
var_dump(vk_whitespace_pack("test \ntest"));
var_dump(vk_whitespace_pack("test \ttest"));
var_dump(vk_whitespace_pack("test\n\ttest"));
var_dump(vk_whitespace_pack("test\t test"));
var_dump(vk_whitespace_pack("test\t  test"));
?>

--EXPECT--
string(4) "test"
string(5) " test"
string(5) " test"
string(6) " ntest"
string(6) " ntest"
string(5) "
test"
string(5) "
test"
string(5) "test "
string(5) "test "
string(5) "test
"
string(5) "test "
string(5) "test
"
string(5) "test "
string(9) "test test"
string(9) "test test"
string(9) "test
test"
string(9) "test test"
string(9) "test
test"
string(9) "test test"
string(9) "test test"

