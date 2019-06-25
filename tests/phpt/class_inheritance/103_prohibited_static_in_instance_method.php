@kphp_should_fail
<?php

class A {
    public static function foo() {
        var_dump("A");
    }

    public function bar() {
        static::foo();
    }
}

class B extends A {
    public static function foo() {
        var_dump("B");
    }
}

/**@var A*/
$a = new A();
$a = new B();
$a->bar();
