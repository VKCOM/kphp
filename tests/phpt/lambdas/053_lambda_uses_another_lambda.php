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

withC(function(A $a) { $a->fa(); }, new A);
withC(function($v) { var_dump($v); }, 100);

