@kphp_should_fail
<?php
require_once 'polyfills.php';

$z = new Classes\Z4BadPhpdoc();
$z->zInst = false;          // ok
$z->zInst = 1;              // not ok
echo $a->a;
