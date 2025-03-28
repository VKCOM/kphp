@ok
<?php

abstract class A {
    public function __construct(int $a) {
        echo "A!" . $a;
    }
}

class B extends A {}

new B(10);
