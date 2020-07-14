@ok
<?php

interface IOne {
    public function one(int $x);
}

interface ITwo extends IOne {
    public function two(int $x, int $y);
}

class ImplTwo implements ITwo {
    public function one(int $x) { var_dump($x); }
    public function two(int $x, int $y) { var_dump($x + $y); }

    public static function one_static(int $z) {}
}

$two = new ImplTwo();
$two->one(10);
$two->two(10, 20);
