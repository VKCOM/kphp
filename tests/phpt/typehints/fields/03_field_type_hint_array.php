@ok php7_4
<?php

class A {
    /** @var A[] */
    public array $instances = [];

    public array $ids = [1,2,3];

    function f() {
        echo "A f\n";
    }
}

$a = new A;
$a->instances[] = new A;
foreach ($a->instances as $i) {
    $i->f();
}
$a->f();
$a->ids[] = '3';
