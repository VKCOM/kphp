@kphp_should_fail
/Could not find ffi_cdata<foo, Bar>/
<?php

/** @param ffi_cdata<foo, struct Bar> $x */
function f($x) {}

f(null);
