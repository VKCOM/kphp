@kphp_should_fail
/Couldn't reify generic <TElem> for call/
<?php

/**
 * @kphp-generic TElem
 * @param TElem[] $arg
 */
function f($arg) {}

f([null]);


