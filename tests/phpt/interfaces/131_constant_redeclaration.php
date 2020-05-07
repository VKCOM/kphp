@kphp_should_fail
<?php

interface A { const CONST_X = 1; }

interface B { const CONST_X = 2; }

class Foo implements A, B {}
class Bar implements B, A {
    public function x() { self::CONST_X; }
}

new Foo;
(new Bar)->x();

