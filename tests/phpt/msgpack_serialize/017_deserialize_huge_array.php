@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

// it's a huge array with 2 ** 31 - 1 elements
$msg = chr(221).chr(0xff).chr(0xff).chr(0xff).chr(0xff).chr(0xff);

@var_dump(msgpack_deserialize($msg));

// causes false positive stack-overflow in asan mode with gcc < 7
//try {
//    msgpack_deserialize_safe($msg);
//} catch (Exception $e) {
//    var_dump("ok");
//}
