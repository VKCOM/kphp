Comparison (operator <) results in PHP 7 and PHP 8 are different for 0 and "foo" (PHP7: false, PHP8: true)
Comparison (operator <) results in PHP 7 and PHP 8 are different for 42 and "42foo" (PHP7: false, PHP8: true)
Comparison (operator <) results in PHP 7 and PHP 8 are different for "100foo" and 42 (PHP7: false, PHP8: true)
<?php

set_show_number_string_conversion_warning(true);

echo 0 < "foo";
echo 42 < "42foo";
echo 42 > "100foo";
