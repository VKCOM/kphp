@kphp_should_fail
/pass boolean to argument \$x of test/
<?php

/**
 * @param $x false
 */
function test($x) {
}

test(true);
