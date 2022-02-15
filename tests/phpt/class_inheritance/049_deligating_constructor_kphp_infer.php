@ok
<?php

class B {
    /**
     * @param $x int
     */
    public function __construct($x) {}
}

class D extends B {
}

class B2 {
    /**
     * @param $x int
     */
    public function __construct($x) {}
}

class D2 extends B2 {
}

$d = new D(10);
$d2 = new D2(10);
var_dump("OK");

class XX {
    /** @param int|null $a */
    function __construct($a = null) {}
}

class YY extends XX {
    function __construct() {
        parent::__construct();
    }
}

class ZZ extends YY {
}

new ZZ;
