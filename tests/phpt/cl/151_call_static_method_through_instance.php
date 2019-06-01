@ok
<?php

class A {
    public function foo() {}
    public static function bar() { var_dump("static A::bar"); }
}


$a = new A();
A::bar();
$a->bar();

