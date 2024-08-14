@k2_skip kphp_runtime_should_warn
/Comparison \(operator <\) results in PHP 7 and PHP 8 are different for 0 and "foo" \(PHP7: false, PHP8: true\)/
/Comparison \(operator <\) results in PHP 7 and PHP 8 are different for 42 and "42foo" \(PHP7: false, PHP8: true\)/
/Comparison \(operator <\) results in PHP 7 and PHP 8 are different for "100foo" and 42 \(PHP7: false, PHP8: true\)/
<?php

set_migration_php8_warning(0b001);

echo 0 < "foo";
echo 42 < "42foo";
echo 42 > "100foo";
