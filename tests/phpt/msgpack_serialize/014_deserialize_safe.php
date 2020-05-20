@ok
<?php

require_once 'polyfills.php';

try {
    msgpack_deserialize_safe('asdfasfdasdfsafd');
} catch (\Exception $e) {
    var_dump($e->getMessage());
}

#ifndef KittenPHP
var_dump("msgpacke_serialize buffer overflow");
#endif

try {
    msgpack_serialize_safe(str_repeat('9', 100000000));
} catch (\Exception $e) {
    var_dump($e->getMessage());
}
