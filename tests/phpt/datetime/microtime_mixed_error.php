@kphp_should_fail
/pass mixed to argument \$x of ensure_string/
<?php

/** @param string $x */
function ensure_string($x) {}

function test() {
  $as_float = true;
  ensure_string(microtime($as_float));
}

test();
