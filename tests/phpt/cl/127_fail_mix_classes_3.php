@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';


$arr = array(new Classes\A);
$arr[] = 4;
