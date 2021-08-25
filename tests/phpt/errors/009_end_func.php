@kphp_should_fail
/KPHP does not support end\(\), maybe array_last_value\(\) is what you are looking for\?/
<?php

$arr = [];
end($arr);
