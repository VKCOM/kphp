@kphp_should_fail
/Calculated generic <T> = int\[\] breaks condition 'T:int\|string'/
<?php

/**
 * @kphp-generic T: int|string
 * @param T $arg
 */
function f($arg) {}

function take($arr) {
    /** @var int[] $arr */
    f($arr);
}

take([1]);
