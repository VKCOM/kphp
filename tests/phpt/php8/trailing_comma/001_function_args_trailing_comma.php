@ok php8
<?php

require_once 'kphp_tester_include.php';

function foo(
  $arg,
  $arg2, // Ok
) {
    echo $arg . $arg2 . "\n";
}

foo("vk", "com");
