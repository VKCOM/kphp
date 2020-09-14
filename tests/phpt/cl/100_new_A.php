@ok
<?php
require_once 'kphp_tester_include.php';

$a = new Classes\A;
$a->a = 126;
$a->printA();

if(!$a->a);
