@kphp_should_fail
/restricted to call Logs125\\BaseLog125::createLog\(\), it's not required by @messages125/
/restricted to call Logs125\\BaseLog125::logAction\(\), it's not required by @messages125/
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

$_ = \Messages125\MessagesLogger125::log();
\Messages125\MessagesLogger125::create();
