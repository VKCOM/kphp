@kphp_should_fail
<?php

function f(...$xs) {}
f(1, , 2); // Empty lvalues are not allowed
