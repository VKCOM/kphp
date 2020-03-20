@ok
<?php

require_once("polyfills.php");

$b = new Classes\PassCallbackInsideClass;
$b->use_callback();
