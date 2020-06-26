@ok
<?php


/**@param callable(int, bool):bool $f*/
function foo($f) {
  var_dump($f(12, false));
}

function demo() {
  foo(function (int $x, bool $b) : bool { return $x % 2 == 0; });
  foo(function (int $x, bool $b) : bool { return $x % 2 == 1; });
}

demo();
