@ok
<?php

abstract class BBB { 
    public function __construct($a) {
        var_dump("BBB-{$a}");
    }
}

abstract class BB extends BBB {
}

class B extends BB {
    public function __construct($x, $y = 9) {
        parent::__construct(10);
        var_dump("B-{$x}-{$y}");
    }
}

class D extends B {
}

$d = new D(7);
