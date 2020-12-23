@ok
<?php

class A {
    function fa() { echo "fa\n"; }
}

/**
 * @kphp-template $callArg
 */
function withC(callable $predicate, $callArg) {
  $inner = function() use ($predicate, $callArg) {
    $predicate($callArg);
  };
  $inner();
}

withC(function($a) { $a->fa(); }, new A);

// BTW, another instantiation doesn't work (and has never worked): using lambdas inside template functions disappoints :(
// withC(function($v) { var_dump($v); }, 100);

