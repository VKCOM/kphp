@ok
<?php

use function VK\Utils\rand;

require __DIR__ . '/include/Utils/vk_rand.php';

function test() {
  var_dump(gettype(\rand()) !== gettype(rand()));
  var_dump([rand(), \VK\Utils\rand()]);
}

test();
