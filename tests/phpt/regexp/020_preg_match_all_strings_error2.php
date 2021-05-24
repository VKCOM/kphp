@kphp_should_fail
/pass mixed to argument \$x of ensure_strings/
<?php

/** @param string[][] $x */
function ensure_strings($x) {}

function test() {
  $pat = '/x/';
  preg_match_all($pat, 'y', $m, PREG_OFFSET_CAPTURE);
  ensure_strings($m);
}

test();
