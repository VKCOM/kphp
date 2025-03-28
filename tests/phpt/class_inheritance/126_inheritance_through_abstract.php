@ok
<?php

class A {
    public function __construct() {
        echo "A!";
    }
}

abstract class B extends A {}

class C extends B {}

new C();
