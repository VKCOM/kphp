@kphp_should_fail
/references to variables in `use` block are forbidden in lambdas/
<?php

function f($y) {
  return function($x) use(&$y) {};
}

f(10);
