@kphp_should_fail
KPHP_ENABLE_FFI=1
/ffi_array_get: invalid \$arr argument/
<?php

$x = FFI::new('int32_t');
ffi_array_get($x, 0);
