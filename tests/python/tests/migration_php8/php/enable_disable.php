Comparison (operator ==) results in PHP 7 and PHP 8 are different for 0 and "" (PHP7: true, PHP8: false)
<?php

set_show_number_string_conversion_warning(true);

echo 0 == ""; // warning

set_show_number_string_conversion_warning(false);

echo 0 == ""; // no warning
