@ok
<?php

function get_value(int $x) : int {
  if ($x == 0) {
    throw new Exception();
  }
  return $x;
}

function f(int $x, int $y) {
  var_dump([$x => $y]);
}

try {
  f(get_value(1), get_value(0));
} catch (Exception $e) {
  var_dump($e->getLine());
}

try {
  f(get_value(0), get_value(1));
} catch (Exception $e) {
  var_dump($e->getLine());
}
