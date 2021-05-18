<?php

namespace Foo;

require_once __DIR__ . '/consts.php';

use const Bar\Const1;

// 'use' is not relative to the current namespace,
// so it will give an error here.
var_dump(Const1);
