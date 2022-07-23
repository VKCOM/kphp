@kphp_should_fail
/Please, provide all generic types using syntax withDef4\/\*<T1, T2>\*\/\(\.\.\.\)/
/Couldn't reify generic <T1> for call/
<?php

class A { function f() { echo "A f\n"; } }

/**
 * @kphp-generic T1, T2 = mixed
 * @param T1 $a
 * @param T2 $b
 */
function withDef4($a, $b = null) {
    if ($a === null) echo "null\n";
    else $a->f();
    var_dump($b);
}

withDef4(null);
