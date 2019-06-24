@kphp_should_fail
/parent/
<?php

class Base {
    public function __construct($x) {
        var_dump($x);
    }
}

class Derived extends Base {
}

$d = new Derived(100);
