@kphp_should_fail
/Foreach reference/
<?php

$a = [1, 2, 3];
foreach ($a as &$x) {}
unset($x);
foreach ($a as $x) {}
