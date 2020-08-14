@ok
<?php

class A { public function foo() {} }

function foo(A $a, callable $run) {
    $a->foo();
}

foo(new A(), function($a) { $a->foo(); });

