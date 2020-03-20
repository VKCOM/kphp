@kphp_should_fail
<?php
require_once 'polyfills.php';


$mix = [new \Classes\A, new Classes\B];

$mix2 = [new Classes\A];
$mix2[] = new Classes\B;
