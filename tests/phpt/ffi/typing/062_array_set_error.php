@kphp_should_fail
KPHP_ENABLE_FFI=1
/ffi_array_set: invalid \$arr argument/
<?php

$x = 0;
ffi_array_set($x, 0, 0);
