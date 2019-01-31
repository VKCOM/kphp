@ok
<?php

function f($x, ...$args) {
    $args[0] = 'a';
    var_dump($args);
}

f(1, 2, 3);
