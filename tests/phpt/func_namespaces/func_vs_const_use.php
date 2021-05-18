@ok
<?php

use function Utils\rand as Random;

require __DIR__ . '/include/Utils/rand.php';

const Random = 10;

function test() {
  var_dump(Random);
  var_dump(Random());
}

test();
