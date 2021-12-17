@kphp_should_fail
/Using 'static' is prohibited from instance methods when a class has derived classes/
<?php

class A {
    public static function foo() {
        var_dump("A");
    }

    public function bar() {
        static::foo();
        echo static::class;
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
