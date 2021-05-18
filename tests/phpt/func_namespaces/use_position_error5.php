@kphp_should_fail
/'use' should be used at the top of the file/
<?php

require_once __DIR__ . '/include/Utils/rand.php';

function my_rand() { return 24; }

use function Utils\rand as my_rand;
