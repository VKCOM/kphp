@ok
<?php

class A { public function foo() { echo "A run\n"; } }

function foo(A $a, callable $run) {
    $a->foo();
    $run($a);
}

foo(new A(), function(A $a) { $a->foo(); });

