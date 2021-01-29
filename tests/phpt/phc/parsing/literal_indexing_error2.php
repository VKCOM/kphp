@kphp_should_fail
/\$x is int, can not get element/
<?php

$x = 1;
var_dump((($x))[0]);
