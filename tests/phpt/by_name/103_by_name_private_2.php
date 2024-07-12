@kphp_should_fail k2_skip
/Can't access private const A::ONE/
<?php

class A {
    private const ONE = 1;
    private static $count = 0;
    private function __construct() {}
    /** @kphp-required */
    private static function sm() { echo 'sm'; }
}

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gGetConst($c) { echo $c::ONE; }


// exposed as a separate test, because constants inlining is done way before checking other access modifiers
gGetConst(A::class);
