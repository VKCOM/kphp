@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

Classes\Common::test1('Test1');
echo Classes\Common::C1."\n";
echo Classes\Common::C2."\n";
echo Classes\Common::$f1."\n";
echo Classes\Common::$f2."\n";
var_dump(Classes\Common::$fa1);
