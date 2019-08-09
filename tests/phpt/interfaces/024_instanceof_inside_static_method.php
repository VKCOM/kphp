@ok
<?php

require_once("polyfills.php");
require_once("Classes/autoload.php");

$x = \Classes\DerivedClassWithMagic::do_magic2();
$y = \Classes\BaseClassWithMagic::do_magic();
var_dump($x + $y);
