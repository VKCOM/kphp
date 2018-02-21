@ok
<?php
require_once 'Classes/autoload.php';

$a = new Classes\A;
$a->a = 126;
$a->printA();

if(!$a->a);
