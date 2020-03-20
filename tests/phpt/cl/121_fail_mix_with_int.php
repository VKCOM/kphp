@kphp_should_fail
<?php
require_once 'polyfills.php';


$m = 1;
$m = new Classes\A;
$m->printA();
