@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

// even if KPHP_REQUIRE_FUNCTIONS_TYPING = 0, given @param are checked

/** @param string $b */
function f1(int $a, $b, $c) {}

f1(0, ['s'], 0);
