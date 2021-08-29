@kphp_should_fail
/KPHP does not support reset\(\), maybe array_first_value\(\) is what you are looking for\?/
<?php

$arr = [];
reset($arr);
