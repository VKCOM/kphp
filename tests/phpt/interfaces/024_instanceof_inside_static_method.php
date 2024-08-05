@ok k2_skip
<?php

require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';

$x = \Classes\DerivedClassWithMagic::do_magic2();
$y = \Classes\BaseClassWithMagic::do_magic();
var_dump($x + $y);
