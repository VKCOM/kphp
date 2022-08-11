@ok
KPHP_ENABLE_MODULITE=1
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

Messages002\Messages002::act();

require_once __DIR__ . '/parent/ParentFuncs.php';
ParentFuncs::testThatCantAccessSubsubchild();
