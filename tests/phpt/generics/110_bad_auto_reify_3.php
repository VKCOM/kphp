@kphp_should_fail
/myVarDump\(\$f->s\);/
/myVarDump\(getContainingAny\(\)\);/
/should be strict, it must not contain 'any' inside/
<?php

class Fields {
    public array $s = ['s'];
}

/** @return tuple(int, any) */
function getContainingAny() {
    return tuple(1, 1);
}

/**
 * @kphp-generic T
 * @param T $some
 */
function myVarDump($some) {
    var_dump($some);
}

$f = new Fields;
myVarDump($f->s);
myVarDump(getContainingAny());
