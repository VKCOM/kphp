@kphp_should_fail
<?php
require_once 'polyfills.php';

$a = new Classes\A;
$a->unexising_prop__ = 126;
$a->printA();