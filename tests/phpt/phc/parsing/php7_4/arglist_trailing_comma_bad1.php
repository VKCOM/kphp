@kphp_should_fail php7_4
<?php

function f(...$xs) {}
f(,); // Free-standing commas are not allowed
