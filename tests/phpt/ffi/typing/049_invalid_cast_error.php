@kphp_should_fail k2_skip
/pass int to argument \$cdata of FFI::cast/
/declared as @param FFI\\CData/
<?php

$int = 10;
$_ = FFI::cast('int', $int);
