@kphp_should_fail
/Function wrongReturn1\(\) returns Unknown type/
/Variable \$a has Unknown type/
/\$a is any, can not get element/
/Function wrongReturn2\(\) returns Unknown type/
<?php

class A {}

/**
 * @return A|int
 */
function wrongReturn1() {
    $m = [];
    for ($i = 0; $i < 1; ++$i)
        $m[] = new A;
    return $m;
}

/**
 * @return A|null[]
 */
function wrongReturn2() {
    $m = [];
    for ($i = 0; $i < 1; ++$i)
        $m[] = new A;
    return $m;
}

$a = wrongReturn1();
if ($a) {
    echo $a[0];
}

$b = wrongReturn2();

