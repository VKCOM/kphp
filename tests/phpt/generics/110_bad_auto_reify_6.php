@kphp_should_fail
/f\(\$i\);/
/Couldn't reify generic <T> for call/
<?php

/**
 * @kphp-generic T
 * @param T $t
 */
function f($t) {
    var_dump($t);
}

function dd(int $i) {
    // this won't work, since param has no assumption, and potentially local var could be smartcasted, re-assigned, etc.
    // (btw, adding @var int $i helps)
    f($i);
}

dd(2);
