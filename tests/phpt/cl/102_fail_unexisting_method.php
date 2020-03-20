@kphp_should_fail
<?php
require_once 'polyfills.php';

$a = new Classes\A;
$a->unexisting_method__();
