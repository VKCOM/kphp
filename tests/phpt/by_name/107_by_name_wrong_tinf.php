@kphp_should_fail k2_skip
KPHP_SHOW_ALL_TYPE_ERRORS=1
/in callByName<A>/
/pass string to argument \$a of A::sf/
/in assignByName<A>/
/assign string to A::\$SIZE/
<?php

class A {
    static public int $SIZE = 0;

    /** @kphp-required */
    static function sf(int $a) {}
}

/**
 * @kphp-generic T
 * @param class-string<T> $class_name
 */
function callByName(string $class_name) {
    $class_name::sf('s');
}

/**
 * @kphp-generic T
 * @param class-string<T> $class_name
 */
function assignByName(string $class_name) {
    $class_name::$SIZE = 's';
}

callByName('A');
assignByName('A');
