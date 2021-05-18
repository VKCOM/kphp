<?php

use const Foo\Bar\Errors\E_WARNING;

require __DIR__ . '/include/error_codes.php';

function test() {
  var_dump(\E_WARNING, E_WARNING);
  var_dump(\Foo\Bar\Errors\E_WARNING);
}

test();
