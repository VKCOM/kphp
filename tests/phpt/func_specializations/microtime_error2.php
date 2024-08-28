@kphp_should_fail k2_skip
/pass float to argument \$x of ensure_string/
/microtime returns float/
<?php

function ensure_string(string $x) {}

function test() {
  ensure_string(microtime(true));
}

test();
