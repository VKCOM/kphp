@kphp_should_fail k2_skip
/pass int to argument \$x of ensure_array/
/hrtime returns int/
<?php

/** @param int[] $x */
function ensure_array($x) {}

function test() {
  ensure_array(hrtime(true));
}

test();
