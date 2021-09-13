@kphp_should_fail php8
/assign string to Point::\$x/
<?php

class Point {
     public function __construct(public int $x, public int $y, public int $z) {}
}

$point = new Point(1, 2, 3);
$point->x = "foo";
