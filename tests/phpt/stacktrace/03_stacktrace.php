@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass string to argument \$a of f/
/\$ii is string/
/implicitly casted to string/
/assign int to \$ii/
/but it's declared as @var string/
/3 is int/
<?php

function f(int $a) {}
/** @var string $ii */
$ii = 3;
f($ii);
