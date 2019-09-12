@kphp_should_fail
/you may not use trait directly/
<?php

trait Tr {
    public static $x = 10;

    public static function foo() {
        var_dump(Tr::$x);
    }
}

class A {
    use Tr;
}

A::foo();
