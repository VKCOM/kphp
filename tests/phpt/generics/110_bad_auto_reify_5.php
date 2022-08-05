@kphp_should_fail
/f\(\$f->x\);/
/Generics <T> = any should be strict, it must not contain 'any' inside/
<?php

class Fields {
    public array $x = [1];
}

/**
 * @kphp-generic T
 * @param T[] $arg
 */
function f($arg) {}

$f = new Fields;
f($f->x);
