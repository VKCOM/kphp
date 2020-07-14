@ok
<?php

interface IWithStatic {
    public static function foo(int $x): int;
}

class A implements IWithStatic {
    public static function foo(int $x): int {
        return $x;
    }
}

class B implements IWithStatic {
    public static function foo(int $x, int $z = 20): int {
        return $x;
    }
}

var_dump(A::foo(10));
var_dump(B::foo(10));

