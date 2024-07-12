@kphp_should_fail k2_skip
KPHP_SHOW_ALL_TYPE_ERRORS=1
/return int from cb/
/but it's declared as @return string/
/return string from cb/
/but a callback was expected to return bool/
/return int from function\(\$v\)/
/but it's declared as @return bool/
/function\(\$v\) should return something, but it is void/
<?php

/** @kphp-required */
function cb($v): string {
    return 5;
}

array_filter([1], 'cb');
array_filter([1], function($v) { return 5; });

array_map(function($v) {}, [1]);
