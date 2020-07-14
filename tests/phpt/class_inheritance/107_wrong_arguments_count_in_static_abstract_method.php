@kphp_should_fail
/Count of arguments/
<?php

abstract class Base {
    abstract public static function foo(int $x);
}

class Derived extends Base {
    public static function foo(int $x, int $y) {
    }
}
