@ok
<?php

require_once 'polyfills.php';

$arr = ['a', 99999999999];
var_dump(msgpack_deserialize(msgpack_serialize($arr)));
