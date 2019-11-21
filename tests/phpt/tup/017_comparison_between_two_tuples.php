@ok
<?php

require_once("polyfills.php");


function same_size_tuples() {
    $a = tuple(1, "2", [1, "2"]);
    $b = tuple(1, "2", [1, "2"]);
    var_dump($a == $b);
    var_dump($a === $b);

    $a = tuple(10, "2", [1, "2"]);
    var_dump($a == $b);
    var_dump($a === $b);
}

function different_size_tuples() {
    $a = tuple(1);
    $b = tuple(1, "2", [1, "2"]);
    var_dump($a == $b);
    var_dump($a === $b);
}

function different_types_tuples() {
    $a = tuple(1, "php");
    $b = tuple(1, 0);
    var_dump($a == $b);
    var_dump($a === $b);
}

function array_and_tuple() {
    $a = tuple(1, 2);
    $b = [1, 2];
    var_dump($a == $b);
    var_dump($b == $a);

    var_dump($a === $b);
    var_dump($b === $a);
}

function array_mixed_and_tuple() {
    $a = tuple(1, 1, [1, 2]);
    $b = ["1", 1, [1, 2]];
    var_dump($a == $b);
    var_dump($b == $a);
    var_dump($a === $b);
    var_dump($b === $a);
}

function int_and_tuple() {
    $a = tuple("1", 1);
    $b = 10;
    var_dump($a == $b);
    var_dump($b == $a);
    var_dump($a === $b);
    var_dump($b === $a);
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
different_size_tuples();
different_types_tuples();
array_and_tuple();
array_mixed_and_tuple();
int_and_tuple();
tuple_of_mixed();
