@ok
<?php

/**
 * @kphp-required
 */
function XXX($n)
{
    return $n * $n;
}

print_r(array_map('XXX', [1, 2, 3, 4]));
