@kphp_should_fail
/rand\(\) should have 0 or 2 arguments/
/mt_rand\(\) should have 0 or 2 arguments/
<?php

$x = rand(1);
$y = mt_rand(2);
