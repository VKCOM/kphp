@ok
<?php

// Same const imported using different local names.

use const VK\Net\Http\CODE_NOT_FOUND as CODE_404;
use const VK\Net\Http\CODE_NOT_FOUND as HTTP_404;

require_once __DIR__ . '/include/http_codes.php';

function test() {
  var_dump(CODE_404);
  var_dump(HTTP_404);
}

test();
