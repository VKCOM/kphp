@kphp_should_fail
/Calculated generic <Object1> = A\[\] breaks condition 'Object1:object'/
/Calculated generic <T> = int breaks condition 'T:object'/
<?php

class A {
    public int $magic = 0;
}

function printMagic(object $arg) {
    var_dump($arg->magic);
    return $arg;
}

/** @var A[] */
$a_arr = [];
printMagic($a_arr);


/**
 * @kphp-generic T: object
 * @param T[] $objects
 */
function stringifyMany($objects) {
    foreach ($objects as $o)
        echo $o, "\n";
}

stringifyMany([1, 2]);
