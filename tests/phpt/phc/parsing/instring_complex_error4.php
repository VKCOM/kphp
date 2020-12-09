@kphp_should_fail
/Expected '}' after get_varname/
<?php

// Works in PHP, not supported in KPHP

function get_varname() { return 'foo'; }

$foo = 15;
var_dump("${get_varname()}");
