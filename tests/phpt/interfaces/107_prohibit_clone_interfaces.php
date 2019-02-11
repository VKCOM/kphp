@kphp_should_fail
<?php

require_once("Classes/autoload.php");

/** @var Classes\IDo $x */
$x = new Classes\A();
$x = new Classes\B();

$y = clone $x;
