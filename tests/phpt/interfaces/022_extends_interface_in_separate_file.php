@ok
<?php

require_once("polyfills.php");

$ab = new Classes\ABImplementsInterfaceB();
$ab->getSelf();
$ab->getSelfAsB();
var_dump("ok");

