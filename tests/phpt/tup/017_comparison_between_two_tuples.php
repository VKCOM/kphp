@ok
<?php

require_once 'polyfills.php';


function same_size_tuples() {
    $a = tuple(1, "2", [1, "2"]);
    $b = tuple(1, "2", [1, "2"]);
    var_dump($a == $b);
    var_dump($a === $b);

    $a = tuple(10, "2", [1, "2"]);
    var_dump($a == $b);
    var_dump($a === $b);
}

function different_types_tuples() {
    $a = tuple(1, "php");
    $b = tuple(1, 0);
    var_dump($a == $b);
    var_dump($a === $b);
}

function tuple_of_mixed() {
    $a = tuple(true ? 1 : "1");
    $b = tuple(1);
    var_dump($a == $b);
    var_dump($b == $a);
    var_dump($a === $b);
    var_dump($b === $a);
}

same_size_tuples();
different_types_tuples();
tuple_of_mixed();
