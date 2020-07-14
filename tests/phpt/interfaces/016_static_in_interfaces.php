@ok
<?php

interface IWithStatic {
   public function foo(): int;
   public static function bar(int $x, int $y): int;
}

class A implements IWithStatic {
    public function foo(): int { return 10; }
    public static function bar(int $x, int $y, int $z = 10): int { return $x + $y; }
}

$a = new A();
var_dump($a->foo());
var_dump(A::bar(10, 20));
