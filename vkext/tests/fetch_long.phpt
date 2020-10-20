--TEST--
test 'fetch_long' function

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';

$s = "\x01\x00\x00\x00\x00\x00\x00\x00";
rpc_parse($s);
var_dump(fetch_long() === 1);

$s = "\x00\x00\x00\x00\x00\x00\x00\x00";
rpc_parse($s);
var_dump(fetch_long() === 0);

$s = "\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00";
rpc_parse($s);
var_dump(fetch_long() === 0);
var_dump(fetch_long() === 1);

$s = "\x00\x00\x00\x00\x00\x00\x00";
rpc_parse($s);
var_dump(@fetch_long() === false);

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
