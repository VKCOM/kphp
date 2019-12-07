@ok
<?php

require_once("polyfills.php");

interface IX {
}

class A implements IX {
    public $x = 10;

    public function __clone() {
        var_dump("CLONED: A");
    }
}

class B implements IX {
    public $y = 20;

    public function __clone() {
        var_dump("CLONED: B");
    }
}

function test(IX $ix) {
    $ix2 = clone $ix;
    if ($ix2 instanceof A) {
        $ix2->x = 12312312;
        var_dump($ix2->x);
        var_dump(instance_cast($ix, A::class)->x);
    } else if ($ix2 instanceof B) {
        $ix2->y = 12312312;
        var_dump($ix2->y);
        var_dump(instance_cast($ix, B::class)->y);
    }
}

test(new A());
test(new B());
