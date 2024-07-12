@kphp_should_fail k2_skip
/ffi_array_get index type must be int, A used instead/
<?php

class A {}

$size = 100;
$arr = FFI::new("uint8_t[$size]");
var_dump(ffi_array_get($arr, new A()));
