@kphp_should_fail
/Syntax '\$class_name::\$method_name\(\)' is invalid in KPHP/
<?php

class A {}

$class_name = 'A';
$method = 'mm';
$class_name::$method(10);

