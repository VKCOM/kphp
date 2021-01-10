@kphp_should_fail
/pass bool to argument \$x of test/
<?php

/**
 * @param $x false
 */
function test($x) {
}

test(true);
