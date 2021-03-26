@ok
<?php

require_once 'kphp_tester_include.php';

/** @param int[] $v */
function arg(array $v): int {
  var_dump($v);
  return array_first_value($v);
}

class Example {
  public static function f(int $x, int $y, int $z): int { return $x; }
  public static function g(int $x, int $y): int { return $x; }
}

function test() {
  Example::f(1, 2, 3);
  Example::f(arg([__LINE__ => 1]), 3, 4);
  Example::f(1 , 2, arg([__LINE__ => 3]));

  Example::g(arg([__LINE__ => 1]), arg([__LINE__ => 2]));
  Example::g(arg([__LINE__ => 2]), arg([__LINE__ => 1]));

  Example::f(Example::g(arg([__LINE__ => 1]), 2), arg([__LINE__ => 3]), 4);
  Example::f(1, Example::g(arg([__LINE__ => 5]), 2), arg([__LINE__ => 4]));

  Example::f(
    Example::g(arg([__LINE__ => 9]), arg([__LINE__ => 8])),
    Example::g(arg([__LINE__ => 7]), arg([__LINE__ => 6])),
    Example::g(arg([__LINE__ => 5]), arg([__LINE__ => 4]))
  );

  Example::f(
    Example::g(
      Example::g(arg([__LINE__ => 100]), arg([__LINE__ => 200])),
      arg([
        __LINE__ => arg(['nested' => 1000])
      ])
    ),
    Example::g(arg([__LINE__ => 7]), arg([__LINE__ => 6])),
    Example::g(arg([__LINE__ => 5]), arg([__LINE__ => 4]))
  );
}

test();
