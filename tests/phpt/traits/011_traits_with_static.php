@ok
<?php

trait T {
    /**
     * @return string
     */
    public static function foo() {
        return 't';
    }
}

class B {
    /**
     * @return string
     */
    public static function foo() {
        return 'b';
    }

    /**
     * @return string
     */
    public static function bar() {
        return static::foo();
    }
}

class A extends B {
    use T;
}

var_dump(A::bar());
var_dump(B::bar());
