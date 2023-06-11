@ok
<?php
require_once 'kphp_tester_include.php';

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

function demo2() {
    $a = tuple(
        1,
        'str',
    );

    /** @var int $int */
    /** @var string $string */

    $int = $a[0];
    $string = $a[1];

    echo "int = ", $int, "\n";
    echo "string = ", $string, "\n";
}

demo();
demo2();
