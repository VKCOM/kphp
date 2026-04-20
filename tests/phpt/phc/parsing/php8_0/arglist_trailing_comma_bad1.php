@kphp_should_fail
<?php

function f(...$xs) {}
f(,); // Free-standing commas are not allowed
