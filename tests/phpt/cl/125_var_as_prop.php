@ok
<?php
require_once 'Classes/autoload.php';

$a = new Classes\A;
$a->a = 'str';
$a->printA();
$a->a = 4.5;
$a->printA();
