@kphp_should_fail
/pass mixed to argument \$x of ensure_strings/
<?php

/** @param string[][] $x */
function ensure_strings($x) {}

function test() {
  preg_match_all('/x/', 'y', $m, PREG_OFFSET_CAPTURE);
  ensure_strings($m);
}

test();
