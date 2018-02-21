@kphp_should_fail
<?php
require_once 'Classes/autoload.php';


$mix = [new \Classes\A, new Classes\B];

$mix2 = [new Classes\A];
$mix2[] = new Classes\B;
