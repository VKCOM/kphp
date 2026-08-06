@ok php8
<?php

require_once "src/fn/Foo.php";
require_once "src/iter/fn/Foo.php";
require_once "src/self/fn/Foo.php";

use fn\Foo as Foo1;
use iter\fn\Foo as Foo2;
use self\Foo as Foo3;

Foo1::run();
Foo2::run();
Foo2::run();
