@kphp_should_fail k2_skip
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass int to argument \$str1 of strcmp/
/but it's declared as @param string/
/pass string\[\] to argument \$ip of long2ip/
/but it's declared as @param int/
<?php
require_once __FILE__ . '.inc';
