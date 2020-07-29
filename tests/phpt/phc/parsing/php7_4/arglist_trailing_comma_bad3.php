@kphp_should_fail php7_4
<?php

function f(...$xs) {}
f(1, , 2); // Empty lvalues are not allowed
