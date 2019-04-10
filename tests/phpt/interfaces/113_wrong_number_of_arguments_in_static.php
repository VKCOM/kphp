@kphp_should_fail
<?php

interface IWithStatic {
    public function foo();
    public static function bar($x, $y);
}

class A implements IWithStatic {
    public function foo() { return 10; }
    public static function bar($x) { return $x; }
}

$a = new A();
var_dump($a->foo());
var_dump(A::bar(10));

