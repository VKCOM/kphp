@kphp_should_fail
KPHP_REQUIRE_FUNCTIONS_TYPING=1
/Specify @param or type hint for \$a/
<?php

// the line after @kphp_should_fail sets env variable, so typing is mandatory

function f1($a) {}

f1(3);
