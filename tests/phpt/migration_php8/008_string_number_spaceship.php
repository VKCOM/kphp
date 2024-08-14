@k2_skip kphp_runtime_should_warn
/Comparison \(operator <\) results in PHP 7 and PHP 8 are different for 0 and "foo" \(PHP7: false, PHP8: true\)/
/Comparison \(operator ==\) results in PHP 7 and PHP 8 are different for 0 and "foo" \(PHP7: true, PHP8: false\)/
/Comparison \(operator <\) results in PHP 7 and PHP 8 are different for 9 and "42foo" \(PHP7: true, PHP8: false\)/
<?php

set_migration_php8_warning(0b001);

echo 0 <=> "foo";   // warnings
echo 9 <=> "42foo"; // warning
echo 0 <=> "10";    // ok
