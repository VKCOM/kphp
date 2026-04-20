@ok php8
<?php

// See https://wiki.php.net/rfc/trailing-comma-function-calls

/**
 * @param $xs array
 * @return int
 */
function f(...$xs) {
  var_dump($xs);
  return count($xs);
}

$lambda = function(...$xs) {
  var_dump($xs);
  return count($xs);
};

$lambda(f(0,),);
$lambda(
  f("x", "y",),
  "z",
);

class C {
  /** @param mixed[] $xs */
  public function __construct(...$xs) {}

  /** @param mixed[] $xs */
  public function f(...$xs) { var_dump($xs); }

  /** @param mixed[] $xs */
  public static function staticF(...$xs) { var_dump($xs); }
}

$c = new C(1, 2,); // OK to use in constructor call
$c->f(4.5,);
$c->f(
  1,
  f(2, 3, 4,),
);

C::staticF(1,);

// Function-like language constructs permit trailing comma as well.
var_dump(isset($c,));
$x = 10;
unset($x,);
