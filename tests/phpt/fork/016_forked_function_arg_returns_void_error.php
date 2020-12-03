@kphp_should_fail
/Using result of void function g/
<?php

function g() { echo "hi"; }

function f($x) { return $x; }

function demo() {
  $res = fork(f(g()));
}

demo();
