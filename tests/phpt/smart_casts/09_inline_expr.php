<?php

interface Shape {
  public function abs(): Shape;
  public function asString(): string;
}

class EmptyShape implements Shape {
  public function abs(): Shape {
    return $this;
  }

  public function asString(): string {
    return "()";
  }
}

class Point2D implements Shape {
  public $x = 0.0;
  public $y = 0.0;

  public function __construct(float $x, float $y) {
    $this->x = $x;
    $this->y = $y;
  }

  public function abs(): Shape {
    return new Point2D(abs($this->x), abs($this->y));
  }

  public function asString(): string {
    return "($this->x, $this->y)";
  }

  public function isZero2(): bool {
    return $this->x == 0.0 && $this->y == 0.0;
  }
}

class Point3D implements Shape {
  public $x = 0.0;
  public $y = 0.0;
  public $z = 0.0;

  public function __construct(float $x, float $y, float $z) {
    $this->x = $x;
    $this->y = $y;
    $this->z = $z;
  }

  public function abs(): Shape {
    return new Point3D(abs($this->x), abs($this->y), abs($this->z));
  }

  public function asString(): string {
    return "($this->x, $this->y, $this->z)";
  }

  public function isZero3(): bool {
    return $this->x == 0.0 && $this->y == 0.0 && $this->z == 0.0;
  }
}

function test_if_cond(Shape $shape) {
  var_dump([__LINE__ => 'begin ' . $shape->asString()]);

  if ($shape instanceof Point2D && true) {
    var_dump([__LINE__ => $shape->x]);
  }

  if ($shape instanceof Point2D && $shape->x == 0.0) {
    var_dump([__LINE__ => $shape->asString()]);
  }
  if ($shape instanceof Point3D && $shape->x == 0.0) {
    var_dump([__LINE__ => $shape->asString()]);
  }

  // Test nested if statements.
  if ($shape instanceof Point2D && !$shape->isZero2()) {
    $abs = $shape->abs();
    var_dump([__LINE__ => $shape->x + $shape->y]);
    if ($abs instanceof Point2D && !$abs->isZero2()) {
      var_dump([__LINE__ => $abs->asString()]);
    }
  }
}

function test_expr(Shape $shape, bool $extra_cond) {
  var_dump([__LINE__ => 'begin ' . $shape->asString()]);

  $result = $shape instanceof Point2D && ($shape->x + $shape->y != 0);
  var_dump([__LINE__ => $result]);

  $result = $shape instanceof Point2D && $shape->isZero2() && $extra_cond;
  var_dump([__LINE__ => $result]);

  $result = $shape instanceof Point3D && !$shape->isZero3() && $extra_cond;
  var_dump([__LINE__ => $result]);

  $result = ($shape instanceof Point2D && $shape->x >= 0) || $extra_cond;
  var_dump([__LINE__ => $result]);

  $ternary_result1 = $shape instanceof Point2D ? $shape->y : 1.5;
  var_dump([__LINE__ => $ternary_result1]);

  $ternary_result2 = $shape instanceof Point3D ? $shape->isZero3() : $shape->asString() === '(0, 0)';
  var_dump([__LINE__ => $ternary_result2]);

  $ternary_result3 = $shape instanceof Point2D ? ($shape->x + $shape->y) : -111;
  var_dump([__LINE__ => $ternary_result3]);
}

$empty = new EmptyShape();
$pt2zero = new Point2D(0, 0);
$pt2 = new Point2D(-14, 29.5);
$pt3zero = new Point3D(0, 0, 0);
$pt3 = new Point3D(1391, -13, -411.4);

test_if_cond($empty);
test_if_cond($pt2zero);
test_if_cond($pt2);
test_if_cond($pt3zero);
test_if_cond($pt3);

foreach ([true, false] as $extra_cond) {
  test_expr($empty, $extra_cond);
  test_expr($pt2zero, $extra_cond);
  test_expr($pt2, $extra_cond);
  test_expr($pt3zero, $extra_cond);
  test_expr($pt3, $extra_cond);
}
