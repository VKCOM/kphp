@kphp_should_fail
<?php
require_once 'polyfills.php';

/** @var Classes\A $a */
$a = 1;
$a->printA();
