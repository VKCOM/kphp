@kphp_should_fail
/function\(\$x\) should return something, but it is void/
<?php

$x = array_map(function ($x) { echo $x . "\n"; }, [1, 2, 3]);
