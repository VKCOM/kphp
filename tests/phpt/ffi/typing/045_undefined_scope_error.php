@kphp_should_fail
KPHP_ENABLE_FFI=1
/Could not find ffi_scope<Foo>/
<?php

/** @param ffi_scope<Foo> $foo */
function f($foo) {
}

f(null);
