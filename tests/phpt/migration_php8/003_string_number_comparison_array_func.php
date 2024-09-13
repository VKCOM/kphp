@kphp_runtime_should_warn k2_skip
/Comparison \(operator ==\) results in PHP 7 and PHP 8 are different for 14.670000 and "14.67foo" \(PHP7: true, PHP8: false\)/
/Comparison \(operator ==\) results in PHP 7 and PHP 8 are different for 20 and "20foo" \(PHP7: true, PHP8: false\)/
/Comparison \(operator <\) results in PHP 7 and PHP 8 are different for 13 and "13foo" \(PHP7: false, PHP8: true\)/
<?php

set_migration_php8_warning(0b001);

$vals = [13, "13foo", "14.67foo", "20foo"];
var_dump(in_array(14.67, $vals));
var_dump(array_search(20, $vals));

sort($vals);
