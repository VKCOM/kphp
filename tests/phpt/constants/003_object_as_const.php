@ok php8
<?php
require_once 'kphp_tester_include.php';

const OBJ = new Classes\A(42);
echo(OBJ);

$tmp = OBJ;
echo($tmp);
