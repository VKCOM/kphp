@ok
<?php

trait A {
    abstract public function run(int $x);
}

trait B {
    abstract public function run(int $x, float $y = 0, string $s = "");
}

trait C {
    abstract public function run(int $x, float $y = 0);
}

abstract class D {
    use A;
    use C;
    use B;
}
