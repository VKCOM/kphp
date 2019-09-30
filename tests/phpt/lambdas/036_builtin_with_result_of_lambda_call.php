@ok
<?php

function run(callable $fetcher, callable $comparator) {
    $res = $fetcher(10);
    usort($res, $comparator);
    var_dump($res[0], $res[1]);
}

run(function ($x) { return [$x - 10, $x]; }, function ($x, $y) { return $x > $y ? -1 : 1; });

