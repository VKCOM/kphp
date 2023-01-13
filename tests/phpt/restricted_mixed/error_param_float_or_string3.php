@kphp_should_fail
/pass false to argument \$x of f/
/declared as @param float\|string/
<?php

/** @param float|string $x */
function f($x) {}

f(false);
