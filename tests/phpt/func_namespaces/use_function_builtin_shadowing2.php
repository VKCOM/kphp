@ok
<?php

use function Utils\rand2;

require __DIR__ . '/include/Utils/rand2.php';

function test() {
  var_dump(gettype(\rand()) !== gettype(rand2()));
  var_dump([\Utils\rand(), rand2(), \Utils\rand2()]);
}

test();
