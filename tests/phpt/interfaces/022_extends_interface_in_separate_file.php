@ok
<?php

require_once("Classes/autoload.php");

$ab = new Classes\ABImplementsInterfaceB();
$ab->getSelf();
$ab->getSelfAsB();
var_dump("ok");

