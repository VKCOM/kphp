@kphp_should_fail
<?php

$x = array_map(function ($x) { echo $x . "\n"; }, [1, 2, 3]);
