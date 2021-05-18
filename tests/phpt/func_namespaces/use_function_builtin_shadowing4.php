@ok
<?php

use VK\Utils;

require __DIR__ . '/include/Utils/vk_rand.php';

function test() {
  var_dump(gettype(\rand()) !== gettype(Utils\rand()));
  var_dump([Utils\rand(), \VK\Utils\rand()]);
}

test();
