@ok
<?php

abstract class BBB { 
    /**
     * @param int $a
     */
    public function __construct($a) {
        var_dump("BBB-{$a}");
    }
}

abstract class BB extends BBB {
}

class B extends BB {
    /**
     * @param int $x
     * @param int $y
     */
    public function __construct($x, $y = 9) {
        parent::__construct(10);
        var_dump("B-{$x}-{$y}");
    }
}

class D extends B {
}

$d = new D(7);
