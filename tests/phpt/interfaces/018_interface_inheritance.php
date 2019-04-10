@ok
<?php

interface IOne {
    public function one($x);
}

interface ITwo extends IOne {
    public function two($x, $y);
}

class ImplTwo implements ITwo {
    public function one($x) { var_dump($x); }
    public function two($x, $y) { var_dump($x + $y); }

    public static function one_static($z) {}
}

$two = new ImplTwo();
$two->one(10);
$two->two(10, 20);
