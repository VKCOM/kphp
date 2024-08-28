@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

$packed_value = msgpack_serialize(10);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize("asdfa");
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize([1, 2]);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize(['a' => 10, 'b' => 20]);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize(null);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize(false);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize(true);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize(123123.123);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize(true ? 123123.123 : false);
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));

$packed_value = msgpack_serialize(true ? [10 => 3, 2] : "adsf");
var_dump(base64_encode($packed_value));
var_dump(msgpack_deserialize($packed_value));
