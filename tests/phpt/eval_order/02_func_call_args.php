@ok
<?php

require_once 'kphp_tester_include.php';

/** @param int[] $v */
function arg(array $v): int {
  var_dump($v);
  return array_first_value($v);
}

function f(int $x, int $y, int $z): int { return $x; }

function g(int $x, int $y): int { return $x; }

function variadic(int ...$args) {}

function test() {
  f(1, 2, 3);
  f(arg([__LINE__ => 1]), 3, 4);
  f(1 , 2, arg([__LINE__ => 3]));

  g(arg([__LINE__ => 1]), arg([__LINE__ => 2]));
  g(arg([__LINE__ => 2]), arg([__LINE__ => 1]));

  f(g(arg([__LINE__ => 1]), 2), arg([__LINE__ => 3]), 4);
  f(1, g(arg([__LINE__ => 5]), 2), arg([__LINE__ => 4]));

  f(
    g(arg([__LINE__ => 9]), arg([__LINE__ => 8])),
    g(arg([__LINE__ => 7]), arg([__LINE__ => 6])),
    g(arg([__LINE__ => 5]), arg([__LINE__ => 4]))
  );

  f(
    g(
      g(arg([__LINE__ => 100]), arg([__LINE__ => 200])),
      arg([
        __LINE__ => arg(['nested' => 1000])
      ])
    ),
    g(arg([__LINE__ => 7]), arg([__LINE__ => 6])),
    g(arg([__LINE__ => 5]), arg([__LINE__ => 4]))
  );

  // More arguments that we can wrap into a macro.
  variadic(
    arg([__LINE__ => 1]),
    arg([__LINE__ => 2]),
    arg([__LINE__ => 3]),
    arg([__LINE__ => 4]),
    arg([__LINE__ => 5]),
    arg([__LINE__ => 6]),
    arg([__LINE__ => 7]),
    arg([__LINE__ => 8]),
    arg([__LINE__ => 9]),
    arg([__LINE__ => 10])
  );
  variadic(
    arg([__LINE__ => 1]),
    arg([__LINE__ => 2]),
    arg([__LINE__ => 3]),
    arg([__LINE__ => 3]),
    arg([__LINE__ => 5]),
    arg([__LINE__ => 1]),
    arg([__LINE__ => 5]),
    arg([__LINE__ => 88]),
    arg([__LINE__ => 1]),
    arg([__LINE__ => 0])
  );
}

test();
