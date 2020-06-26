@ok
<?php

/**@var (callable(int):int)[] $a */
$a = [
    0 => function($x) { return $x; },
    function($y) { return 10 + $y; },
];

foreach ($a as $f) {
    var_dump($f(9));
}
