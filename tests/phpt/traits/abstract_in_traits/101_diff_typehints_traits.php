@kphp_should_fail
/Declaration of B::foo must be compatible with version in trait A/
<?php

trait A { abstract public function foo(int $x); }

trait B { abstract public function foo(float $x); }

abstract class AB {
    use A;
    use B;
}
