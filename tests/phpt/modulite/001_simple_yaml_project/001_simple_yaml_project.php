@ok
KPHP_ENABLE_MODULITE=1
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

$lc = Utils001\Strings001::lowercase("HELLO");
echo $lc, "\n";

if (0) new \GlobalA001;
if (0) new \GlobalB001;
Utils001\Strings001::hasPhpdoc(null);
