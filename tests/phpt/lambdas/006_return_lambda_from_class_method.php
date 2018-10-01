@ok
<?php

require_once("Classes/autoload.php");

$a = new Classes\CallbackHolder();
$fun = $a->get_callback();

var_dump($fun(10, 20));
