@ok
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

Messages002\Messages002::act();

require_once __DIR__ . '/parent/ParentFuncs_002.php';
ParentFuncs_002::testThatCantAccessSubsubchild();
