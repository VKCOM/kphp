@kphp_should_fail
<?php
require_once 'polyfills.php';

/** @var Classes\B $a */
$a = new Classes\A;
$a->getThis();
