@kphp_should_fail
<?php
require_once 'Classes/autoload.php';


$m = 1;
$m = new Classes\A;
$m->printA();
