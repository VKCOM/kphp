@kphp_should_fail
/empty\(\) must have exactly one argument/
<?php

var_dump(empty($x, $y));
?>
