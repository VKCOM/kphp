@kphp_should_fail
/lambda can't implement interface: IntChecker/
<?php

interface Checker {
}

interface IntChecker extends Checker {
    public function __invoke(int $x, bool $b) : bool;
}

function foo(IntChecker $f) {
  var_dump($f(12, false));
}

function demo() {
  foo(function (int $x, bool $b) : bool { return $x % 2 == 0; });
  foo(function (int $x, bool $b) : bool { return $x % 2 == 1; });
}

demo();
