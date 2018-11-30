@todo has_race
<?php

function f() {
    $c = function() { return 10; };
    return function() use ($c) { return $c(); };
}

function g() {
    $c = f();
    return $c();
}

var_dump(g());

