@kphp_should_fail
/'use' should be used at the top of the file/
<?php

require __DIR__ . '/include/Utils/rand.php';

$c = new C();
use function Utils\rand;
var_dump([gettype(\rand()), gettype(rand())]);

class C {}
