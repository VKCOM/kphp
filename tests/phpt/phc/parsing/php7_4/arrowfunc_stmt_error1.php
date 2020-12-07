@kphp_should_fail
/Bad expression in arrow function body/
<?php

$fn = fn() => return 10;
var_dump($fn());
