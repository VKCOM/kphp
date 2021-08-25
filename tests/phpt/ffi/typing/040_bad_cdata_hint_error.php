@kphp_should_fail
KPHP_ENABLE_FFI=1
/parse ffi_cdata<C, Foo >: syntax error, unexpected IDENTIFIER/
<?php

/** @return ffi_cdata<C, Foo> */
function f() {}

f();
