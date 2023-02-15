@kphp_should_fail
<?php

function f(...$xs) {}
f(1, 2,,); // Multiple trailing commas are not allowed
