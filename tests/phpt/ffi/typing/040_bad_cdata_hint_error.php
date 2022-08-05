@kphp_should_fail
/parse ffi_cdata<C, Foo >: syntax error, unexpected IDENTIFIER/
<?php

/** @return ffi_cdata<C, Foo> */
function f() {}

f();
