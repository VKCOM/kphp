@kphp_should_fail
<?php

class A {
    public function foo() {}
    /** @kphp-required */
    public static function bar() { var_dump("static A::bar"); }
}


$a = new A();
A::bar();
$a->bar();

