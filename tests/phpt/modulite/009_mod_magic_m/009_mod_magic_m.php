@ok
KPHP_ENABLE_MODULITE=1
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

use Logic009\TestMagic009;

TestMagic009::testToString();
TestMagic009::testCtor();
TestMagic009::testClone();
