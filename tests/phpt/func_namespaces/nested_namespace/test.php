<?php

namespace Foo;

require_once __DIR__ . '/func.php';

use function Foo\Bar\f as foo_bar_f;

var_dump(foo_bar_f());
var_dump(Bar\f());
var_dump(\Foo\Bar\f());
