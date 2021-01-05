@kphp_should_fail
/Type Error/
<?php

$a = array();
$a[0][] = 1;
$a[0][] = tuple(1, tuple(1));

