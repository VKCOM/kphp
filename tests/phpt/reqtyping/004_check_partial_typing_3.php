@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

// even if KPHP_REQUIRE_FUNCTIONS_TYPING = 0, given @return is checked

/** @return int|false */
function f1($a) { return $a; }

f1(4);
f1(null);
