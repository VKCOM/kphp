@kphp_should_fail
KPHP_ENABLE_FFI=1
/ffi casting a non-scalar type char\[\] to a scalar type int32_t/
<?php

$size = 4;
$arr = FFI::new("char[$size]");
FFI::cast('int32_t', $arr);
