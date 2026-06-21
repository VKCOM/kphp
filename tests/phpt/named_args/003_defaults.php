@ok php8
<?php

function test1($a = 'a', $b = 'b') {
    echo "a: $a, b: $b\n";
}

function test2($a = SOME_CONST, $b = 'b') {
    echo "a: $a, b: $b\n";
}

test1(b: 'B');

define('SOME_CONST', 'X');
test2(b: 'B');
