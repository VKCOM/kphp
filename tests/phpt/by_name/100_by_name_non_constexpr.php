@kphp_should_fail k2_skip
/Syntax 'new \$class_name' works only if \$class_name is compile-time known/
/Syntax '\$class_name::method\(\)' works only if \$class_name is compile-time known/
/Syntax '\$class_name::CONST' works only if \$class_name is compile-time known/
/Syntax '\$class_name::\$field' works only if \$class_name is compile-time known/
<?php

class A {}

$class_name = 'A';
new $class_name;
$class_name::hello();
$class_name::A;
$class_name::$bo;
