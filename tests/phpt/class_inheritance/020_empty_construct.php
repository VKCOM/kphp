@ok
<?php

class B {
    public function __construct() {
        var_dump("B");
    }
}

class D extends B {
    public function __construct() {
    }
}

$d = new D();

