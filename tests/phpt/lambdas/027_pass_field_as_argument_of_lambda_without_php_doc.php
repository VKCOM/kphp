@ok
<?php

require_once("Classes/autoload.php");

$callback = function ($x) { var_dump($x); };
$i = new Classes\ImplicitCapturingThis(100);
$i->pass_field_as_argument_of_lambda($callback);


