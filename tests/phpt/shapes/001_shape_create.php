@ok
<?php
require_once 'polyfills.php';

function demo() {
    $a = shape(['i' => 1, 's' => 'str']);

    // check that types are correctly inferred
    /** @var int $int */
    /** @var string $string */

    $int = $a['i'];
    $string = $a['s'];

    echo "int = ", $int, "\n";
    echo "string = ", $string, "\n";
}

demo();
