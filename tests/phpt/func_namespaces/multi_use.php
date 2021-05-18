@ok
<?php

// Same function imported using different local names.

use function VK\funcname as funcname2;
use function VK\funcname as funcname3;

require_once __DIR__ . '/include/funcname.php';

function funcname() { return ['global', __FUNCTION__]; }

function test() {
  var_dump(funcname());
  var_dump(funcname2());
  var_dump(funcname3());
}

test();
