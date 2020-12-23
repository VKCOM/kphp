@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/return int from cb/
/but it's declared as @return string/
/return string from cb/
/but a callback was expected to return boolean/
/return int from lambda/
/but a callback was expected to return boolean/
/Expression as expression:  return ...  at function: anonymous\(...\) is not allowed to be void/
<?php

/** @kphp-required */
function cb($v): string {
    return 5;
}

array_filter([1], 'cb');
array_filter([1], function($v) { return 5; });

array_map(function($v) {}, [1]);
