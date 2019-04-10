@ok
<?php

interface IWithStatic {
   public function foo();
   public static function bar($x, $y);
}

class A implements IWithStatic {
    public function foo() { return 10; }
    public static function bar($x, $y, $z = 10) { return $x + $y; }
}

$a = new A();
var_dump($a->foo());
var_dump(A::bar(10, 20));
