@ok
<?php

interface IWithStatic {
    public static function foo($x);
}

class A implements IWithStatic {
    public static function foo($x) {
        return $x;
    }
}

class B implements IWithStatic {
    public static function foo($x, $z = 20) {
        return $x;
    }
}

var_dump(A::foo(10));
var_dump(B::foo(10));

