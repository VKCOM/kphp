@ok
<?php

trait T {
    public static function foo() {
        return 't';
    }
}

class B {
    public static function foo() {
        return 'b';
    }

    public static function bar() {
        return static::foo();
    }
}

class A extends B {
    use T;
}

var_dump(A::bar());
var_dump(B::bar());
