@kphp_should_fail
/Invalid place for unpack, because param \$a is @kphp-infer cast/
/Invalid place for unpack, because param \$s is @kphp-infer cast/
/Invalid place for unpack, because param \$i is @kphp-infer cast/
<?php

function f($a ::: array) {
}
f(...[[1, 2]]);

/**
 * @kphp-infer cast
 * @param string $s
 * @param int $i
 */
function g($s, $i) {
}
g(...['s', 1]);
