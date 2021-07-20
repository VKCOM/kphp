@kphp_should_fail
/Declaration of A::foo must be compatible with version in trait B/
<?php

trait A {
    abstract public function foo(int $x);
}

trait B {
    abstract public function foo(int $x, int $y = 0);
}

abstract class AB {
    use B;
    use A;
}
