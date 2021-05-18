@ok
<?php

// Like the other test, but uses a "function use alias".

use function Utils\rand as my_rand;

require __DIR__ . '/include/Utils/rand.php';

function test() {
  var_dump(gettype(\rand()) !== gettype(my_rand()));
  var_dump(gettype(rand()) !== gettype(my_rand()));
  var_dump(my_rand() === \Utils\rand());
}

test();
