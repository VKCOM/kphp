@kphp_should_fail
/Can not override/
<?php
require_once 'Classes/autoload.php';
use Classes\Inheritance\Child11;
use Classes\Inheritance\Child1;
use Classes\Inheritance\Base;

Base::const_test4();
echo "----\n";
Child1::const_test4();
echo "----\n";
Child11::const_test4();
