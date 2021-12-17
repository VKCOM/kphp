@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass Lambda\$/
/to argument \$call_me of foo/
/pass int to argument \$call_me of foo/
/but it's declared as @param callable\(int\):bool/
<?php

/**
 * @param callable(int):bool $call_me
 */
function foo($call_me) {
  var_dump($call_me(10));
}

function demo() {
  $f = function (string $x) : bool { return true; };
  foo($f);
  foo(5);
}

demo();
