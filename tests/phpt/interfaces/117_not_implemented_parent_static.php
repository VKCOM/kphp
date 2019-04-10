@kphp_should_fail
<?php
interface IOne {
    public function one($x);
    public static function static_one($x, $y);
}

interface ITwo extends IOne {
    public function two($x, $y);
    public static function static_two($u);
}
class ImplTwo implements ITwo {
    public function one($x) { var_dump($x); }
    public function two($x, $y) { var_dump($x + $y); }
    public static function static_one($x, $y) { var_dump("impl_two"); }
}
