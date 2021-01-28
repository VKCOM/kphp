@kphp_should_fail
/Expected '\)', found '\['/
/Failed to parse statement. Expected `;`/
<?php

$x = 'a';
var_dump("$x"[0]); // forbidden in PHP as well
