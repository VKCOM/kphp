@k2_skip kphp_runtime_should_warn
/Comparison \(operator ==\) results in PHP 7 and PHP 8 are different for 0 and "" \(PHP7: true, PHP8: false\)/
<?php

set_migration_php8_warning(0b001);

echo 0 == ""; // warning

set_migration_php8_warning(0b000);

echo 0 == ""; // no warning
