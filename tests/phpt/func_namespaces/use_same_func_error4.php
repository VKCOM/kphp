@kphp_should_fail
/Cannot declare function my_rand because the name is already in use/
<?php

require_once __DIR__ . '/include/Utils/rand.php';

use function Utils\rand as my_rand;

function my_rand() { return 24; }
