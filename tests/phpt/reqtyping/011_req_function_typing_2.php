@kphp_should_fail
KPHP_REQUIRE_FUNCTIONS_TYPING=1
/return int from f3/
<?php

// if KPHP_REQUIRE_FUNCTIONS_TYPING = 1, then assume @return void unless specified

function f3() { return 5; }

f3();
