@kphp_should_fail
/unexpected token, expected C type, '>' or '\)'/
<?php

/** @return ffi_cdata<C, int+> */
function f() {}

f();
