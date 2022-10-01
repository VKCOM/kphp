@kphp_should_fail
/Deduce implicit types and casts/
/vg\(null, 1, 's'\);/
/Couldn't reify generic <T1> for call/
/Please, provide all generic types using syntax vg\/\*<T1, TArgs1, TArgs2>\*\/\(\.\.\.\)/
<?php

/**
 * @kphp-generic T1, ...TArgs
 * @param T1 $first
 * @param TArgs ...$args
 */
function vg($first, ...$args) {
    echo count($args);
}

vg(null, 1, 's');
