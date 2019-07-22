@kphp_should_fail
<?php

$x = function() { return 5;};
$y = array_map($x, [1, 2, 3]);

