@ok
<?php

class Base {
    /**
     * @kphp-infer
     * @param int $x
     */
    public function __construct($x) {
        var_dump("Base", $x);
    }

    public function foo() {
        var_dump("Base");
    }
}

class Derived extends Base {
    /**
     * @kphp-infer
     * @param int $x
     */
    public function __construct($x) {
        parent::__construct($x);
        var_dump("Derived", $x);
    }

    public function foo() {
        // parent::foo();
        var_dump("dervived::foo");
    }
}

$d = new Derived(123);
