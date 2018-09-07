@ok
<?php
require_once 'polyfill/tuple-php-polyfill.php';

function demo() {
    $a = tuple(1, 'str');

    $int = $a[0];
    $string = $a[1];

    // check that types are correctly inferred
    $int /*:= int */;
    $string /*:= string */;

    echo "int = ", $int, "\n";
    echo "string = ", $string, "\n";
}

demo();
