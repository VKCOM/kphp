@kphp_should_fail k2_skip
/Invalid php2c conversion in this context: FFI\\CData_char\* -> uint8_t/
<?php

$size = 4;
$arr = FFI::new("uint8_t[$size]");
ffi_array_set($arr, 0, FFI::new('char*'));
