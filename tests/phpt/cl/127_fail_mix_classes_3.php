@kphp_should_fail
<?php
require_once 'polyfills.php';


$arr = array(new Classes\A);
$arr[] = 4;
