@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

// even if KPHP_REQUIRE_FUNCTIONS_TYPING = 0, given type hints are checked

function f1(int $a, $b, $c) {}

f1('s', 0, 0);
