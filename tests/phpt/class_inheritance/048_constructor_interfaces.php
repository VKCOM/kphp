@ok
<?php

interface I {
    public function __construct(int $x);
}

class A implements I {
     public function __construct(int $x, float $y = 10) {}
}

$a = new A(7);
