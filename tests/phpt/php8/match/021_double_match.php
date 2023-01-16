@ok php8
<?php

function f($int) {
    return match (match ($int) {
        1 => "one",
        2 => "two",
        default => "many",
    }) {
        "one " => 1,
        "two" => 2,
        default => 42,
    };
}

echo f(1)  . "\n";
echo f(2)  . "\n";
echo f(42) . "\n";
