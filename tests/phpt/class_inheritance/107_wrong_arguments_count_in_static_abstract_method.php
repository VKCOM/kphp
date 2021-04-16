@kphp_should_fail
/Declaration of Base::foo must be compatible with version in class Derived/
<?php

abstract class Base {
    abstract public static function foo(int $x);
}

class Derived extends Base {
    public static function foo(int $x, int $y) {
    }
}
