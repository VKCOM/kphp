@ok
<?php

use function Utils\rand;

require __DIR__ . '/include/Utils/rand.php';

function test() {
  // This calls the builtin rand (returns int) and our own rand that returns a string.
  var_dump(gettype(\rand()) !== gettype(rand()));

  // Calls the same function using the FQN and its local name.
  var_dump([rand(), \Utils\rand()]);
}

test();
