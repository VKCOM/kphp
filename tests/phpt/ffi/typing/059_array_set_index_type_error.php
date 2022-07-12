@kphp_should_fail
KPHP_ENABLE_FFI=1
/ffi_array_set index type must be int, float used instead/
<?php

class A {}

$size = 100;
$k = 1.5;
$arr = FFI::new("int32_t[$size]");
ffi_array_set($arr, $k, 0);
