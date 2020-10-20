--TEST--
test 'fetch_string' function

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';

$s = "\x01a";
rpc_parse($s);
var_dump(fetch_string() === "a");

$s = "\x02ab";
rpc_parse($s);
var_dump(fetch_string() === "ab");

$s = "\x02a";
rpc_parse($s);
var_dump(@fetch_string() === false);

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
