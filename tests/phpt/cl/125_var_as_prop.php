@ok
<?php
require_once 'kphp_tester_include.php';

$a = new Classes\A;
$a->a = 'str';
$a->printA();
$a->a = 4.5;
$a->printA();
