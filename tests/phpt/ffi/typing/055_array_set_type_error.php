@kphp_should_fail
KPHP_ENABLE_FFI=1
/Invalid php2c conversion in this context: string -> uint8_t/
<?php

$size = 4;
$arr = FFI::new("uint8_t[$size]");
ffi_array_set($arr, 0, "");
