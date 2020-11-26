@kphp_should_fail
/Function calls on the left-hand side of \?\?= are not supported/
<?php

class A {
    /** @var ?int */
    public $x = null;

    /** @var A */
    public $y = null;

    public function next() : A {
      return null;
    }
}

$a = new A();

$a->y->next()->y->x ??= 99;
