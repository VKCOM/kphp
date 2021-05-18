@ok
<?php

use VK\Utils as VKUtils;

require __DIR__ . '/include/Utils/vk_rand.php';

function test() {
  var_dump(gettype(\rand()) !== gettype(VKUtils\rand()));
  var_dump([VKUtils\rand(), \VK\Utils\rand()]);
}

test();
