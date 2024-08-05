@kphp_should_fail k2_skip
/Variable \$x has Unknown type/
<?php

var_dump(array_filter(array(), function($x) { return $x - 1 > 0; }));

