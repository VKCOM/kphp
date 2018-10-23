@kphp_should_fail
<?php

var_dump(array_filter(array(), function($x) { return $x - 1 > 0; }));

