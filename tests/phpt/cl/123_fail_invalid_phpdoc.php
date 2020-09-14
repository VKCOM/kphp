@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

/** @var Classes\B $a */
$a = new Classes\A;
$a->getThis();
