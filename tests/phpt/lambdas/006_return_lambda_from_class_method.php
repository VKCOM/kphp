@ok
<?php

require_once("polyfills.php");

$a = new Classes\CallbackHolder();
$fun = $a->get_callback();

var_dump($fun(10, 20));
