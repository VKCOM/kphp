@ok
<?php

require_once("polyfills.php");

/** @var Classes\IDo $can_do */
$can_do = new Classes\A();
$can_do->do_it(10, 20);

$can_do = new Classes\B();
$can_do->do_it(1000, 2000);

