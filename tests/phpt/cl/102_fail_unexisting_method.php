@kphp_should_fail
<?php
require_once 'Classes/autoload.php';

$a = new Classes\A;
$a->unexisting_method__();
