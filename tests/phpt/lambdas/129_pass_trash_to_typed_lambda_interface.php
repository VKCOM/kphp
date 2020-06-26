@kphp_should_fail
/Can't infer lambda from expression passed as argument: call_me/
/You must passed lambda as argument: call_me/
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
