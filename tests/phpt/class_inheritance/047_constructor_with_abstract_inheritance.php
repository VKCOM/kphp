@ok
<?php

abstract class BaseBase {
    public abstract function __construct(int $x);
}

abstract class Base extends BaseBase {
}

abstract class Derived extends Base {
    public function __construct(int $x) { var_dump("Derived"); }
}

class D extends Derived {}

$d = new D(10);

