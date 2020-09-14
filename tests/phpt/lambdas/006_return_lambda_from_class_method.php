@ok
<?php

require_once 'kphp_tester_include.php';

$a = new Classes\CallbackHolder();
$fun = $a->get_callback();

var_dump($fun(10, 20));
