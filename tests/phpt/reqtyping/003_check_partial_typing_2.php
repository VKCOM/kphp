@kphp_should_fail
/pass array< string > to argument \$b of f1/
/but it's declared as @param string/
<?php

// even if KPHP_REQUIRE_FUNCTIONS_TYPING = 0, given @param are checked

/** @param string $b */
function f1(int $a, $b, $c) {}

f1(0, ['s'], 0);
