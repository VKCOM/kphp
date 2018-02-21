@kphp_should_fail
<?php
require_once 'Classes/autoload.php';

$a = new Classes\A;
$a->unexising_prop__ = 126;
$a->printA();