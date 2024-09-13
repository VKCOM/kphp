@kphp_should_fail k2_skip
/Can't access private static field count/
/Can't access private static method A::sm/
/Can't access private method A::__construct/
<?php

class A {
    private static $count = 0;
    private function __construct() {}
    /** @kphp-required */
    private static function sm() { echo 'sm'; }
}

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gCtor($c) { new $c; }

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gCallMethod($c) { $c::sm(); }

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gGetField($c) { $c::$count++; }

gCtor(A::class);
gCallMethod(A::class);
gGetField(A::class);
