@kphp_should_fail k2_skip
/ffi_array_get: invalid \$arr argument/
<?php

$x = FFI::new('int32_t');
ffi_array_get($x, 0);
