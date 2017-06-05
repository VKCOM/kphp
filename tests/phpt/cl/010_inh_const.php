@ok
<?php
require_once 'Classes/autoload.php';
use Classes\Inheritance\Child11;
use Classes\Inheritance\Child1;
use Classes\Inheritance\Base;

Base::const_test2();
echo "----\n";
Child1::const_test2();
echo "----\n";
Child11::const_test2();
