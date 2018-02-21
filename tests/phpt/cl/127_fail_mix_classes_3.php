@kphp_should_fail
<?php
require_once 'Classes/autoload.php';


$arr = array(new Classes\A);
$arr[] = 4;
