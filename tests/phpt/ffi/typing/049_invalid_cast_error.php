@kphp_should_fail
KPHP_ENABLE_FFI=1
/pass int to argument \$cdata of FFI::cast/
/declared as @param FFI\\CData/
<?php

$int = 10;
$_ = FFI::cast('int', $int);
