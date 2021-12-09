@ok
<?php

trait A {
    abstract public function foo(int $x);
}

trait B {
    abstract public function foo(int $x, int $y = 0);
}

abstract class AB {
    use A;
    use B;
}
