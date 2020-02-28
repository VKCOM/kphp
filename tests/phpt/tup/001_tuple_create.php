@ok
<?php
require_once 'polyfills.php';

function demo() {
    $a = tuple(1, 'str');

    // check that types are correctly inferred
    /** @var int $int */
    /** @var string $string */

    $int = $a[0];
    $string = $a[1];

    echo "int = ", $int, "\n";
    echo "string = ", $string, "\n";
}

demo();
