@ok
<?php

require_once 'polyfills.php';

/**
 * @kphp-infer hint check
 * @kphp-template $a, $b
 * @param int $c
 */
function sum_and_print($c, $a, $b) {
    $a->magic = 98;
    var_dump($c + $a->magic + $b->magic);
}

$a = new \Classes\A;
sum_and_print(1000, $a, $a);

$b = new \Classes\B;
sum_and_print(1000, $b, $b);
