@ok
<?php
interface IOne {
    public function one(int $x);
    public static function static_one(int $x, int $y);
}

interface ITwo extends IOne {
    public function two(int $x, int $y);
    public static function static_one(int $x, int $y);
}
