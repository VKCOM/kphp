<?php

namespace Foo;

require_once __DIR__ . '/consts.php';

use const Foo\Bar\Const1;

var_dump(Const1);
var_dump(Bar\Const1, Bar\Const2);
var_dump(\Foo\Bar\Const1, \Foo\Bar\Const2);
