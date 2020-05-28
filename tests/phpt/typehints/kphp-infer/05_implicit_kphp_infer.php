@kphp_should_fail
/Specify @param for arguments: \$y \$x/
<?php

function foo(int $x, $y) {}

foo(10, 20);
