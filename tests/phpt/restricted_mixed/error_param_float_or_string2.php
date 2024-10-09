@kphp_should_fail
/pass bool to argument \$x of f/
/declared as @param float\|string/
<?php

/** @param float|string $x */
function f($x) {}

function test() {
  $b = true;
  f($b);
}

test();
