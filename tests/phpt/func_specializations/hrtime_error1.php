@kphp_should_fail k2_skip
/pass mixed to argument \$x of ensure_array/
<?php

/** @param int[] $x */
function ensure_array($x) {}

function test() {
  $as_int = true;
  ensure_array(hrtime($as_int));
}

test();
