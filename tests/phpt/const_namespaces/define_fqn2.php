@ok
<?php

require_once __DIR__ . '/include/define_fqn_in_namespace.php';

use const Foo\C1 as FooC1;

var_dump(\Foo\C1);
var_dump(FooC1);
