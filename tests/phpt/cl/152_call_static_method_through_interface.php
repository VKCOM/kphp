@kphp_should_fail
/static method through interface/
<?php

interface IA {
    public static function bar();
}

class A implements IA {
    public function foo() {}
    public static function bar() { var_dump("static A::bar"); }
}

/** @var IA */
$a = new A();
$a->bar();

