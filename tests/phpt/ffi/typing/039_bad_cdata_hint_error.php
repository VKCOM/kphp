@kphp_should_fail
KPHP_ENABLE_FFI=1
/unexpected token, expected C type, '>' or '\)'/
<?php

/** @return ffi_cdata<C, int+> */
function f() {}

f();
