@kphp_should_fail
<?php

$y = 10;
$change_y = function() use (&$y) { $y = 100; }; 
$change_y();
var_dump($y);


