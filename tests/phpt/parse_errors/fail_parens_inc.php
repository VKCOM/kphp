@kphp_should_fail
/Expected variable, found parenthesized expression/
<?php

$x = 10;
var_dump(($x)++);
