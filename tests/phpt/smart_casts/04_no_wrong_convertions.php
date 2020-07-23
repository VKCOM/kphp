@ok
<?php

/**
 * @kphp-infer
 * @return mixed[]
 */
function run() {
    $a = [null, false, 0, 0.0, [], "", true, 1, 1.0, "abc", [1, 2, 3]];
    $b = [];
    foreach ($a as $x) {
       if ($x) { $b[] = $x; }
       if (!$x) { $b[] = $x; }
       if (is_scalar($x)) { $b[] = $x; }
       if (is_numeric($x)) { $b[] = $x; }
       if (is_null($x)) { $b[] = $x; }
       if (is_bool($x)) { $b[] = $x; }
       if (is_int($x)) { $b[] = $x; }
       if (is_integer($x)) { $b[] = $x; }
       if (is_long($x)) { $b[] = $x; }
       if (is_float($x)) { $b[] = $x; }
       if (is_double($x)) { $b[] = $x; }
       if (is_real($x)) { $b[] = $x; }
       if (is_string($x)) { $b[] = $x; }
       if (is_array($x)) { $b[] = $x; }
    }

    var_dump($b);
    return $b;
}

run();
