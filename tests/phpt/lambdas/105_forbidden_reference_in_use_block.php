@kphp_should_fail
/references to variables in `use` block are forbidden in lambdas/
<?php

$y = 10;
$change_y = function() use (&$y) { $y = 100; }; 
$change_y();
var_dump($y);


