<?php

namespace Example;

require_once __DIR__ . '/file1.php';

use Foo\Bar;
use const Foo\Bar\Baz;

const C1 = Baz + 1;
const C2 = \Foo\Bar\Baz + 2;
const C3 = Bar\Baz + 3;
const C4 = E_WARNING;
