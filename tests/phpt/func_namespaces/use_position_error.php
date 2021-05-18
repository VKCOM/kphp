@kphp_should_fail
/'use' should be used at the top of the file/
<?php

require __DIR__ . '/include/Utils/rand.php';

var_dump([gettype(\rand()), gettype(rand())]);
use function Utils\rand;
var_dump([gettype(\rand()), gettype(rand())]);
