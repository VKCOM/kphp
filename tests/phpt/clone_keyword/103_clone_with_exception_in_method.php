@kphp_should_fail
<?php

class A {
    public function __clone() {
        throw new Exception("asdf");
    }
}

$a = new A();
$b = clone $a;
