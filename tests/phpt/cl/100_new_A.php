@ok
<?php
require_once 'polyfills.php';

$a = new Classes\A;
$a->a = 126;
$a->printA();

if(!$a->a);
