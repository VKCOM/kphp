@kphp_should_fail
<?php

class A {
    public function __construct($x) {
        var_dump($x);
    }

    public function __destruct() {

    }
}

$a = new A(10);

