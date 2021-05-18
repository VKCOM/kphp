@ok
<?php

require_once __DIR__ . '/Foo.php';

use A\B;

var_dump(\A\B\C\Foo::X);
var_dump(A\B\C\Foo::X);
var_dump(B\C\Foo::X);
