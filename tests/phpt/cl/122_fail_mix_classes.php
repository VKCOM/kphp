@kphp_should_fail
<?php
require_once 'Classes/autoload.php';


$a = 1 ? new Classes\A : new Classes\B;
