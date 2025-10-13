@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

$a = new Classes\A;
$a->unexising_prop__ = 126;
$a->printA();
