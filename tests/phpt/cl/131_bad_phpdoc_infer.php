@kphp_should_fail
<?php
require_once 'polyfills.php';

$z = new Classes\Z4BadPhpdoc();
$z->a = '123';          // mixing string and int, whereas /** @var int */ in class
echo $a->a;
