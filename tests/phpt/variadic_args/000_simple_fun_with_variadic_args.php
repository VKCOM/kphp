@ok
<?php

function get_args(...$args) {
    var_dump($args);
}

get_args(1, 2, 3);
get_args([1], "a", [1 => 'a']);
get_args("abc");
get_args();
