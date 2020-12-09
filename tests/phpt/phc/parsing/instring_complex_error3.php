@kphp_should_fail
/Variable name expected/
<?php

// Works in PHP, not supported in KPHP

$foo = 15;
var_dump("${'foo'}");
