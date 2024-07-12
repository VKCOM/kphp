@ok k2_skip
<?php
require_once 'kphp_tester_include.php';
use Classes\Common;

Common::test1('Test1');
echo Common::C1."\n";
echo Common::C2."\n";
echo Common::$f1."\n";
echo Common::$f2."\n";
var_dump(Common::$fa1);
