@kphp_should_fail k2_skip
/in callByName<A>/
/\$class_name::sf\(\);/
/Too few arguments in call to A::sf\(\), expected 1, have 0/
<?php

class A {
    static function sf(int $a) {}
}

/**
 * @kphp-generic T
 * @param class-string<T> $class_name
 */
function callByName(string $class_name) {
    $class_name::sf();
}

callByName('A');

