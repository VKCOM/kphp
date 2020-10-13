@kphp_should_fail
/Specify @param or type hint for \$y/
<?php

function foo(int $x, $y) {}

foo(10, 20);
