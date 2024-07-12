@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

try {
    msgpack_deserialize_safe('asdfasfdasdfsafd');
} catch (\Exception $e) {
    var_dump($e->getMessage());
}

#ifndef KPHP
var_dump("msgpacke_serialize buffer overflow");
#endif

try {
    msgpack_serialize_safe(str_repeat('9', 100000000));
} catch (\Exception $e) {
    var_dump($e->getMessage());
}

/** @kphp-serializable */
class A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $x = 10;

    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $y = 10;
}

try {
    instance_deserialize_safe('asdfasfdasdfsafd', A::class);
} catch (\Exception $e) {
    var_dump($e->getMessage());
}
