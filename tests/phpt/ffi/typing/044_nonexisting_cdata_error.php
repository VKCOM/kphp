@kphp_should_fail
KPHP_ENABLE_FFI=1
/Could not find ffi_cdata<foo, Bar>/
<?php

/** @param ffi_cdata<foo, struct Bar> $x */
function f($x) {}

f(null);
