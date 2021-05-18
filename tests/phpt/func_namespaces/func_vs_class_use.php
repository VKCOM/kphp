@ok
<?php

use function Utils\rand as Random;

require __DIR__ . '/include/Utils/rand.php';

class Random {}

function test() {
  $v = new Random();
  var_dump(Random());
}

test();
