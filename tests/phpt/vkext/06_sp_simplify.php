@ok
<?php

var_dump(vk_sp_simplify("abc123") === "abc123");
var_dump(vk_sp_simplify(" abc123") === "abc123");
var_dump(vk_sp_simplify("\nabc123") === "abc123");
var_dump(vk_sp_simplify("\tabc123") === "abc123");
var_dump(vk_sp_simplify("\x01abc123") === "abc123");
var_dump(vk_sp_simplify("abc123 ") === "abc123");
var_dump(vk_sp_simplify("abc 123") === "abc123");
var_dump(vk_sp_simplify("\x01abc123\t") === "abc123");
var_dump(vk_sp_simplify("\x01abc\n\n\t123\t") === "abc123");

?>

--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)