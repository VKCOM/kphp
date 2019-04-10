@kphp_should_fail
<?php
interface IOne {
    public function one($x);
    public static function static_one($x, $y);
}

interface ITwo extends IOne {
    public function two($x, $y);
    public static function static_one($x, $y);
}
