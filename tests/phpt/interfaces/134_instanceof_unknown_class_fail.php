@kphp_should_fail
/Unknown class at the right of 'instanceof'/
<?php

class A {}

$a = new A;
$is = $a instanceof ABC;
