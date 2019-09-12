@ok
<?php

trait HasStaticInt {
    public static $int = 20;

    public static function inc_int() {
        self::$int += 1;
    }

    public function inc_int_method() {
        self::inc_int();
        var_dump(get_class($this));
    }
}

class A {
    use HasStaticInt;
}

$a = new A();
var_dump(A::$int);

$a->inc_int_method();
var_dump(A::$int);

A::inc_int();
var_dump(A::$int);
