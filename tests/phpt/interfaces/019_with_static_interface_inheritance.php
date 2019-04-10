@ok
<?php

interface IOne {
    public function one($x);
    public static function static_one($x, $y);
}

interface ITwo extends IOne {
    public function two($x, $y);
}

interface IThree extends IOne {
    public function three($x, $y);
}

class ImplTwo implements ITwo {
    public function one($x) { var_dump($x); }
    public function two($x, $y) { var_dump($x + $y); }
    public static function static_one($x, $y) { var_dump("impl_two"); }
}

class ImplThree implements IThree {
    public function one($x) { var_dump($x); }
    public function three($x, $y) { var_dump($x + $y); }
    public static function static_one($x, $y) { var_dump("impl_three"); }
}

/** @var IOne $one */
$one = new ImplTwo();
$one->one(10);

$one = new ImplThree();
$one->one(10000);
