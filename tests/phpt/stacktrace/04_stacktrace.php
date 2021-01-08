@kphp_should_fail
/assign tuple<array< int > , mixed , A> to A::\$t/
/but it's declared as @var tuple<array< int > , int , A>/
/getTuple returns tuple<array< int > , mixed , A>/
/getTuple inferred that returns tuple<array< int > , mixed , A>/
/tuple<> is tuple<array< int > , mixed , A>/
/\$s is mixed/
/\$arr\[\.\] is mixed/
/\$arr is array< mixed >/
/argument \$arr inferred as array< mixed >/
/getArr returns array< mixed >/
/getArr inferred that returns array< mixed >/
/\$arr is array< mixed >/
/array is array< mixed >/
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
