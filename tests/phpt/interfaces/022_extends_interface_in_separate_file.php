@ok
<?php

require_once 'kphp_tester_include.php';

$ab = new Classes\ABImplementsInterfaceB();
$ab->getSelf();
$ab->getSelfAsB();
var_dump("ok");

