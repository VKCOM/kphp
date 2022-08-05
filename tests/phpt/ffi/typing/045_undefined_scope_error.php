@kphp_should_fail
/Could not find ffi_scope<Foo>/
<?php

/** @param ffi_scope<Foo> $foo */
function f($foo) {
}

f(null);
