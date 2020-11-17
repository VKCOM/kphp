@kphp_should_fail
/Expected '}' after arr/
/Expected '}' after xs/
<?php

// These two work in PHP, but are currently rejected by the KPHP

$arr = ['key' => 'val'];
$xs = [1, 2];

var_dump("${arr['key']}");
var_dump("-${xs[0]}-");
