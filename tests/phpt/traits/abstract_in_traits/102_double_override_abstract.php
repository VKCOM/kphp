@kphp_should_fail
/Declaration of C::run must be compatible with version in trait B/
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
    use B;
    use C;
}
