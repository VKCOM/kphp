@kphp_should_fail
/assign tuple\(int\[\], mixed, A\) to A::\$t/
/but it's declared as @var tuple\(int\[\], int, A\)/
/getTuple returns tuple\(int\[\], mixed, A\)/
/getTuple inferred that returns tuple\(int\[\], mixed, A\)/
/tuple<> is tuple\(int\[\], mixed, A\)/
/\$s is mixed/
/\$arr\[\.\] is mixed/
/\$arr is mixed\[\]/
/argument \$arr inferred as mixed\[\]/
/getArr returns mixed\[\]/
/getArr inferred that returns mixed\[\]/
/\$arr is mixed\[\]/
/array is mixed\[\]/
/\$sample is string/
/"str" is string/
<?php

class A {
    /** @var tuple(int[], int, A) */
    var $t;
}

function getTuple($arr) {
    $s = $arr[0];
    return tuple([(int)$s], $s, new A);
}

function getArr(): array {
    $sample = 'str';
    $arr = [1, 2, $sample];
    return $arr;
}

$a = new A;
$a->t = getTuple([1, 2]);
$a->t = getTuple(getArr());
