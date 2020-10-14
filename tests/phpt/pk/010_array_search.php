@ok
<?php


/**
 * @param int $y
 */
function f($y) {
  global $x;
  if (array_search($y, $x) !== false) {
    echo "$y found\n";
  } else {
    echo "$y not found\n";
  }
}

/**
 * @param int $y
 * @param bool $strict
 */
function g($y, $strict) {
  global $x;
  if (array_search($y, $x, $strict) !== false) {
    echo "$y found\n";
  } else {
    echo "$y not found\n";
  }
}

$x = array(1, "2", false);

f(0);
f(1);
f(2);

g(0, false);
g(1, false);
g(2, false);

g(0, true);
g(1, true);
g(2, true);
