@ok
<?php

// Additional regression for #68: instanceof with a class that is
// referenced nowhere else in the translation unit.

interface Shape {}

class Circle implements Shape {
  public float $radius;
  public function __construct(float $r) { $this->radius = $r; }
}

class Square implements Shape {
  public float $side;
  public function __construct(float $s) { $this->side = $s; }
}

// Triangle is declared but never instantiated — only appears in instanceof.
class Triangle implements Shape {}

function describe(Shape $s): string {
  if ($s instanceof Circle) return 'circle';
  if ($s instanceof Square) return 'square';
  if ($s instanceof Triangle) return 'triangle';  // Triangle: only in instanceof
  return 'unknown';
}

var_dump(describe(new Circle(1.0)));   // string(6) "circle"
var_dump(describe(new Square(2.0)));   // string(6) "square"
