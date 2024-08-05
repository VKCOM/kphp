@kphp_should_fail k2_skip
/Invalid php2c conversion in this context: &cdata\$test\\Example -> struct Example/
<?php

$cdef = FFI::cdef('
#define FFI_SCOPE "test"
  struct Example { int32_t x; };
');
$size = 4;
$arr1 = $cdef->new("struct Example[$size]");
$arr2 = $cdef->new("struct Example[$size]");
ffi_array_set($arr1, 0, ffi_array_get($arr2, 0));
