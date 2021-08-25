@kphp_should_fail
KPHP_ENABLE_FFI=1
/parse ffi_cdata<C, struct int8_t >: syntax error, unexpected INT8, expecting \{ or IDENTIFIER or TYPEDEF_NAME/
<?php

/** @return ffi_cdata<C, struct int8_t> */
function f() {}

f();
