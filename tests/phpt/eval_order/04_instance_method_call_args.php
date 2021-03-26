@ok
<?php

require_once 'kphp_tester_include.php';

/** @param int[] $v */
function arg(array $v): int {
  var_dump($v);
  return array_first_value($v);
}

class Example {
  public function f(int $x, int $y, int $z): int { return $x; }
  public function g(int $x, int $y): int { return $x; }
}

function test() {
  $e = new Example();

  $e->f(1, 2, 3);
  $e->f(arg([__LINE__ => 1]), 3, 4);
  $e->f(1 , 2, arg([__LINE__ => 3]));

  $e->g(arg([__LINE__ => 1]), arg([__LINE__ => 2]));
  $e->g(arg([__LINE__ => 2]), arg([__LINE__ => 1]));

  $e->f($e->g(arg([__LINE__ => 1]), 2), arg([__LINE__ => 3]), 4);
  $e->f(1, $e->g(arg([__LINE__ => 5]), 2), arg([__LINE__ => 4]));

  $e->f(
    $e->g(arg([__LINE__ => 9]), arg([__LINE__ => 8])),
    $e->g(arg([__LINE__ => 7]), arg([__LINE__ => 6])),
    $e->g(arg([__LINE__ => 5]), arg([__LINE__ => 4]))
  );

  $e->f(
    $e->g(
      $e->g(arg([__LINE__ => 100]), arg([__LINE__ => 200])),
      arg([
        __LINE__ => arg(['nested' => 1000])
      ])
    ),
    $e->g(arg([__LINE__ => 7]), arg([__LINE__ => 6])),
    $e->g(arg([__LINE__ => 5]), arg([__LINE__ => 4]))
  );
}

test();
