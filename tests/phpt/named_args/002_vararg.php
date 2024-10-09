@ok php8
<?php

function foo(int $a, int $b, ...$args) {
    var_dump($args);
    var_dump($a);
    var_dump($b);
}

foo(a : 1, b : 2, args2: [3, 4, 5]);
foo(1, b : 2, args2: [3, 4, 5]);
foo(1, 2, args2 : [3, 4, 5]);

