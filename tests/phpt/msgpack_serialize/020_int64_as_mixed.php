@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

$arr = ['a', 99999999999];
var_dump(msgpack_deserialize(msgpack_serialize($arr)));
