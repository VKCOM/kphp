@kphp_should_fail k2_skip
/pass mixed to argument \$x of ensure_string/
<?php

function ensure_string(string $x) {}

function test() {
  $as_float = true;
  ensure_string(microtime($as_float));
}

test();
