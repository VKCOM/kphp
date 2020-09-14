@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';


$a = 1 ? new Classes\A : new Classes\B;
