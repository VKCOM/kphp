@kphp_should_fail php8
/Can't access private field z/
<?php

class Point {
     public function __construct(public int $x, public int $y, private int $z) {}
}

$point = new Point(1, 2, 3);
echo $point->x;
echo $point->z;
