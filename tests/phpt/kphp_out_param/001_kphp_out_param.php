@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

// Test that with kphp-out-param we don't get "var may be uninitialized" warnings.

/**
 * @kphp-out-param $x
 * @param int $x
 */
function out_param(&$x) {
  $x = 10;
}

function test() {
  out_param($v);
  var_dump($v);
}

test();
