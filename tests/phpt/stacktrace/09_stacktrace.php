@kphp_should_fail
/insert string into A::\$ids\[\]/
<?php

class A {
    /** @var int[] */
    public array $ids = [1,2,3];

    function f($x) {
        $this->ids[] = $x;
    }
}

$a = new A;
$v = '3';
$a->f($v);
