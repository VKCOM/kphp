@ok php8
<?php

class Point {
     public function __construct(
         public float $x = 0.0,
         public float $y = 1.0,
         public float $z = 2.0
     ) {}

     public function print() {
        echo $this->x . "\n";
        echo $this->y . "\n";
        echo $this->z . "\n";
     }
}

(new Point(10.0))->print();
(new Point(10.0, 11.0))->print();
(new Point(10.0, 11.0, 12.0))->print();
