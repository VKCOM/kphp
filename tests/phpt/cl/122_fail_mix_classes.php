@kphp_should_fail
<?php
require_once 'polyfills.php';


$a = 1 ? new Classes\A : new Classes\B;
